#include "libreavix.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zlib.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#define MAX_PARAMS 10
#define MAX_PLUGINS 20
#define MAX_MIDDLEWARE 10
#define WS_FRAME_SIZE 4096

// Internal structures
typedef struct {
    char* method;
    char* path;
    RouteHandler handler;
} RouteEntry;

typedef struct {
    uv_stream_t* stream;
    bool is_websocket;
    bool websocket_connected;
} ClientContext;

static struct {
    RouteEntry* routes;
    size_t route_count;
    size_t route_capacity;
    TrieNode root_node;
    ErrorHandler error_handler;
    LogConfig log_config;
    SecurityPolicy security_policy;
    Plugin plugins[MAX_PLUGINS];
    size_t plugin_count;
    Middleware middleware[MAX_MIDDLEWARE];
    size_t middleware_count;
    uint8_t enabled_protocols;
    pthread_mutex_t mutex;
    uv_loop_t* loop;
    uv_tcp_t server;
    ClientContext* clients;
    size_t client_capacity;
} reavix_state;

// Compression implementation
static bool compress_data(const char* input, size_t input_len, char** output, size_t* output_len, CompressionType type) {
    if (type == COMPRESSION_NONE || !input || !output) return false;

    z_stream strm;
    int ret;
    size_t max_output = compressBound(input_len);
    *output = malloc(max_output);
    if (!*output) return false;

    if (type == COMPRESSION_GZIP) {
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY);
    } else {
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    }

    if (ret != Z_OK) {
        free(*output);
        return false;
    }

    strm.next_in = (Bytef*)input;
    strm.avail_in = input_len;
    strm.next_out = (Bytef*)*output;
    strm.avail_out = max_output;

    ret = deflate(&strm, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&strm);
        free(*output);
        return false;
    }

    *output_len = max_output - strm.avail_out;
    deflateEnd(&strm);
    return true;
}

// Rate limiting implementation
static bool check_rate_limit(const char* path, const char* client_ip) {
    // TODO: Implement actual rate limiting logic
    // This would typically use a token bucket or sliding window algorithm
    return true;
}

// Initialize the framework
bool reavix_init(size_t max_routes) {
    if (max_routes == 0) return false;

    memset(&reavix_state, 0, sizeof(reavix_state));
    reavix_state.route_capacity = max_routes;
    reavix_state.routes = malloc(max_routes * sizeof(RouteEntry));
    if (!reavix_state.routes) return false;

    pthread_mutex_init(&reavix_state.mutex, NULL);
    reavix_state.enabled_protocols = PROTOCOL_HTTP; // HTTP enabled by default
    
    // Initialize root node
    reavix_state.root_node.segment = strdup("");
    reavix_state.root_node.handler = NULL;
    reavix_state.root_node.children = NULL;
    reavix_state.root_node.param_child = NULL;
    reavix_state.root_node.child_count = 0;
    
    return true;
}

// Route registration with rate limiting
bool reavix_route(const char* method, const char* path, RouteHandler handler) {
    if (!method || !path || !handler) return false;

    pthread_mutex_lock(&reavix_state.mutex);

    if (reavix_state.route_count >= reavix_state.route_capacity) {
        pthread_mutex_unlock(&reavix_state.mutex);
        return false;
    }

    // Check for duplicate routes
    for (size_t i = 0; i < reavix_state.route_count; ++i) {
        if (strcmp(reavix_state.routes[i].method, method) == 0 && 
            strcmp(reavix_state.routes[i].path, path) == 0) {
            pthread_mutex_unlock(&reavix_state.mutex);
            return false;
        }
    }

    // Add to flat route list
    reavix_state.routes[reavix_state.route_count].method = strdup(method);
    reavix_state.routes[reavix_state.route_count].path = strdup(path);
    reavix_state.routes[reavix_state.route_count].handler = handler;
    reavix_state.route_count++;

    // Add to trie router
    if (!trie_insert(&reavix_state.root_node, path, handler)) {
        pthread_mutex_unlock(&reavix_state.mutex);
        return false;
    }

    pthread_mutex_unlock(&reavix_state.mutex);
    return true;
}

// Server implementation
void reavix_server(uint16_t port) {
    reavix_state.loop = uv_default_loop();
    uv_tcp_init(reavix_state.loop, &reavix_state.server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", port, &addr);

    uv_tcp_bind(&reavix_state.server, (const struct sockaddr*)&addr, 0);
    
    int r = uv_listen((uv_stream_t*)&reavix_state.server, SOMAXCONN, on_connection);
    if (r) {
        reavix_log(LOG_FATAL, NULL, "Listen error: %s", uv_strerror(r));
        return;
    }

    reavix_log(LOG_INFO, NULL, "Server running on port %d", port);
    uv_run(reavix_state.loop, UV_RUN_DEFAULT);
}

// Connection handler
static void on_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        reavix_log(LOG_ERROR, NULL, "Connection error: %s", uv_strerror(status));
        return;
    }

    uv_tcp_t* client = malloc(sizeof(uv_tcp_t));
    uv_tcp_init(reavix_state.loop, client);
    
    if (uv_accept(server, (uv_stream_t*)client) == 0) {
        // Add to client tracking
        pthread_mutex_lock(&reavix_state.mutex);
        if (reavix_state.client_capacity == 0) {
            reavix_state.clients = malloc(10 * sizeof(ClientContext));
            reavix_state.client_capacity = 10;
        } else if (reavix_state.client_count >= reavix_state.client_capacity) {
            reavix_state.client_capacity *= 2;
            reavix_state.clients = realloc(reavix_state.clients, 
                reavix_state.client_capacity * sizeof(ClientContext));
        }
        
        ClientContext* ctx = &reavix_state.clients[reavix_state.client_count++];
        ctx->stream = (uv_stream_t*)client;
        ctx->is_websocket = false;
        ctx->websocket_connected = false;
        
        pthread_mutex_unlock(&reavix_state.mutex);
        
        uv_read_start((uv_stream_t*)client, alloc_buffer, on_read);
    } else {
        uv_close((uv_handle_t*)client, NULL);
    }
}

// Request handling
static void handle_request(Request* req, Response* res) {
    // Apply middleware
    for (size_t i = 0; i < reavix_state.middleware_count; i++) {
        bool continue_processing = true;
        reavix_state.middleware[i](req, res, [](Request* r, Response* resp) {
            // Next function implementation
        });
        
        if (res->_internal.headers_sent) {
            return;
        }
    }

    // Apply pre-handler plugins
    for (size_t i = 0; i < reavix_state.plugin_count; i++) {
        if (reavix_state.plugins[i].pre_handler) {
            reavix_state.plugins[i].pre_handler(req, res);
            if (res->_internal.headers_sent) {
                return;
            }
        }
    }

    // Route handling
    PathParam path_params[MAX_PARAMS];
    size_t path_param_count = 0;
    RouteHandler route_handler = NULL;

    pthread_mutex_lock(&reavix_state.mutex);
    bool is_matched = trie_match(&reavix_state.root_node, req->path, 
                                path_params, &path_param_count, &route_handler);
    pthread_mutex_unlock(&reavix_state.mutex);

    if (is_matched && route_handler) {
        // Set path parameters
        req->_internal.param_names = malloc(path_param_count * sizeof(char*));
        req->_internal.param_values = malloc(path_param_count * sizeof(char*));
        req->_internal.param_count = path_param_count;

        for (size_t i = 0; i < path_param_count; i++) {
            req->_internal.param_names[i] = path_params[i].name;
            req->_internal.param_values[i] = path_params[i].value;
        }

        route_handler(req, res);
    } else {
        res_send_error(res, 404, "Not Found");
    }

    // Apply post-handler plugins
    for (size_t i = 0; i < reavix_state.plugin_count; i++) {
        if (reavix_state.plugins[i].post_handler) {
            reavix_state.plugins[i].post_handler(req, res);
        }
    }
}

// Response implementation
void res_send_json(Response* res, const char* json) {
    if (!res || !json) return;

    free(res->content);
    res->content = strdup(json);
    res->content_length = strlen(json);
    res_set_header(res, "Content-Type", "application/json");
    
    if (res->_internal.compression != COMPRESSION_NONE) {
        char* compressed;
        size_t compressed_len;
        if (compress_data(res->content, res->content_length, &compressed, &compressed_len, res->_internal.compression)) {
            free(res->content);
            res->content = compressed;
            res->content_length = compressed_len;
            
            const char* encoding = "";
            switch (res->_internal.compression) {
                case COMPRESSION_GZIP: encoding = "gzip"; break;
                case COMPRESSION_BROTLI: encoding = "br"; break;
                case COMPRESSION_DEFLATE: encoding = "deflate"; break;
                default: break;
            }
            res_set_header(res, "Content-Encoding", encoding);
        }
    }
    
    send_response(res);
}

void send_response(Response* res) {
    if (!res || res->_internal.headers_sent) return;
    
    ClientContext* ctx = find_client_context(res->_internal.client);
    if (!ctx) return;

    // Build headers
    char headers[4096];
    int headers_len = snprintf(headers, sizeof(headers), "HTTP/1.1 %d %s\r\n", 
        res->status_code, get_status_text(res->status_code));
    
    for (size_t i = 0; i < res->_internal.header_count; i++) {
        headers_len += snprintf(headers + headers_len, sizeof(headers) - headers_len, 
            "%s: %s\r\n", res->_internal.header_names[i], res->_internal.header_values[i]);
    }
    
    headers_len += snprintf(headers + headers_len, sizeof(headers) - headers_len, 
        "Content-Length: %zu\r\n\r\n", res->content_length);
    
    // Send headers
    uv_buf_t buf = uv_buf_init(headers, headers_len);
    uv_write_t* write_req = malloc(sizeof(uv_write_t));
    uv_write(write_req, ctx->stream, &buf, 1, on_write_complete);
    
    // Send body if exists
    if (res->content_length > 0 && res->content) {
        uv_buf_t body_buf = uv_buf_init(res->content, res->content_length);
        uv_write_t* body_req = malloc(sizeof(uv_write_t));
        uv_write(body_req, ctx->stream, &body_buf, 1, on_write_complete);
    }
    
    res->_internal.headers_sent = true;
}

// Plugin system
void reavix_register_plugin(Plugin plugin) {
    if (reavix_state.plugin_count >= MAX_PLUGINS) return;
    
    pthread_mutex_lock(&reavix_state.mutex);
    reavix_state.plugins[reavix_state.plugin_count++] = plugin;
    
    if (plugin.init) {
        plugin.init();
    }
    pthread_mutex_unlock(&reavix_state.mutex);
}

// Middleware support
void reavix_use(Middleware middleware) {
    if (reavix_state.middleware_count >= MAX_MIDDLEWARE) return;
    
    pthread_mutex_lock(&reavix_state.mutex);
    reavix_state.middleware[reavix_state.middleware_count++] = middleware;
    pthread_mutex_unlock(&reavix_state.mutex);
}

// WebSocket support
void reavix_ws_send(Response* res, const char* message) {
    if (!res || !message || !res->_internal.client) return;
    
    ClientContext* ctx = find_client_context(res->_internal.client);
    if (!ctx || !ctx->is_websocket) return;
    
    size_t msg_len = strlen(message);
    size_t frame_len = msg_len + 10; // Max frame header size
    char* frame = malloc(frame_len);
    
    // Simple text frame (no masking)
    frame[0] = 0x81; // FIN + text frame
    if (msg_len <= 125) {
        frame[1] = msg_len;
        memcpy(frame + 2, message, msg_len);
        frame_len = msg_len + 2;
    } else if (msg_len <= 65535) {
        frame[1] = 126;
        frame[2] = (msg_len >> 8) & 0xFF;
        frame[3] = msg_len & 0xFF;
        memcpy(frame + 4, message, msg_len);
        frame_len = msg_len + 4;
    } else {
        // For very large messages (unlikely in this context)
        frame[1] = 127;
        // 64-bit length would go here
        // But we'll just error for this example
        free(frame);
        return;
    }
    
    uv_buf_t buf = uv_buf_init(frame, frame_len);
    uv_write_t* write_req = malloc(sizeof(uv_write_t));
    uv_write(write_req, ctx->stream, &buf, 1, on_write_complete);
}

// Compression support
void res_compress(Response* res, CompressionType type) {
    if (!res) return;
    res->_internal.compression = type;
}

// Rate limiting configuration
void reavix_set_rate_limits(const char* path, RateLimitConfig config) {
    pthread_mutex_lock(&reavix_state.mutex);
    
    // Find the route in the trie and set rate limiting
    TrieNode* current = &reavix_state.root_node;
    char* path_copy = strdup(path);
    if (!path_copy) {
        pthread_mutex_unlock(&reavix_state.mutex);
        return;
    }
    
    char* saveptr;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    
    while (segment) {
        bool found = false;
        // Traverse static children first
        for (size_t i = 0; i < current->child_count; i++) {
            if (strcmp(current->children[i].segment, segment) == 0) {
                current = &current->children[i];
                found = true;
                break;
            }
        }
        
        // Traverse parameter child if static not found
        if (!found && current->param_child) {
            current = current->param_child;
            found = true;
        }
        
        if (!found) {
            free(path_copy);
            pthread_mutex_unlock(&reavix_state.mutex);
            return;
        }
        
        segment = strtok_r(NULL, "/", &saveptr);
    }
    
    // Set rate limit configuration in the trie node
    current->rate_limit_config = config;

    free(path_copy);
    pthread_mutex_unlock(&reavix_state.mutex);
}

// Security policy configuration
void reavix_set_security_policy(SecurityPolicy policy) {
    pthread_mutex_lock(&reavix_state.mutex);
    reavix_state.security_policy = policy;
    pthread_mutex_unlock(&reavix_state.mutex);
}

// Protocol enablement
void reavix_enable_protocol(uint8_t protocol) {
    pthread_mutex_lock(&reavix_state.mutex);
    reavix_state.enabled_protocols |= protocol;
    pthread_mutex_unlock(&reavix_state.mutex);
}

// Helper functions
static const char* get_status_text(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

static ClientContext* find_client_context(uv_stream_t* stream) {
    for (size_t i = 0; i < reavix_state.client_count; i++) {
        if (reavix_state.clients[i].stream == stream) {
            return &reavix_state.clients[i];
        }
    }
    return NULL;
}


static bool trie_insert(TrieNode* root, const char* path, RouteHandler handler) {
    if (!root || !path) return false;

    char* path_copy = strdup(path);
    if (!path_copy) return false;

    char* saveptr;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    TrieNode* current = root;

    while (segment) {
        bool is_param = (segment[0] == ':');
        TrieNode** target = is_param ? &current->param_child : NULL;
        bool found = false;

        if (!is_param) {
            for (size_t i = 0; i < current->child_count; i++) {
                if (strcmp(current->children[i].segment, segment) == 0) {
                    current = &current->children[i];
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            TrieNode* new_node = create_trie_node(segment);
            if (!new_node) {
                free(path_copy);
                return false;
            }

            if (is_param) {
                current->param_child = new_node;
            } else {
                current->child_count++;
                current->children = realloc(current->children, current->child_count * sizeof(TrieNode));
                if (!current->children) {
                    free_trie_node(new_node);
                    free(path_copy);
                    return false;
                }
                current->children[current->child_count - 1] = *new_node;
                free(new_node);
            }
            current = is_param ? current->param_child : &current->children[current->child_count - 1];
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    current->handler = handler;
    free(path_copy);
    return true;
}

static bool trie_match(TrieNode* root, const char* path, PathParam params[], 
                      size_t* param_count, RouteHandler* out_handler) {
    if (!root || !path || !param_count || !out_handler) return false;

    char* path_copy = strdup(path);
    if (!path_copy) return false;

    char* saveptr;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    TrieNode* current = root;

    size_t param_index = 0;
    while (segment) {
        bool is_param = (segment[0] == ':');
        TrieNode** target = is_param ? &current->param_child : NULL;
        bool found = false;

        if (!is_param) {
            for (size_t i = 0; i < current->child_count; i++) {
                if (strcmp(current->children[i].segment, segment) == 0) {
                    current = &current->children[i];
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            if (is_param && current->param_child && param_index < MAX_PARAMS) {
                params[param_index].name = current->param_child->segment + 1;
                params[param_index].value = strdup(segment);
                param_index++;
                current = current->param_child;
            } else {
                free(path_copy);
                return false;
            }
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    free(path_copy);
    *param_count = param_index;
    *out_handler = current->handler;
    return current->handler != NULL;
}

// Memory management
void request_free(Request* req) {
    if (!req) return;
    
    for (size_t i = 0; i < req->_internal.header_count; i++) {
        free((void*)req->_internal.header_names[i]);
        free((void*)req->_internal.header_values[i]);
    }
    
    free(req->_internal.header_names);
    free(req->_internal.header_values);
    
    for (size_t i = 0; i < req->_internal.param_count; i++) {
        free((void*)req->_internal.param_names[i]);
        free((void*)req->_internal.param_values[i]);
    }
    
    free(req->_internal.param_names);
    free(req->_internal.param_values);
    free((void*)req->body);
    free(req);
}

void response_free(Response* res) {
    if (!res) return;
    
    free(res->content);
    
    for (size_t i = 0; i < res->_internal.header_count; i++) {
        free(res->_internal.header_names[i]);
        free(res->_internal.header_values[i]);
    }
    
    free(res->_internal.header_names);
    free(res->_internal.header_values);
    free(res);
}