#include "libreavix.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_PARAMS 10
//internal route storage
typedef struct{
    char* method;
    char* path;
    RouteHandler handler;
}RouteEntry;

static RouteEntry* routes = NULL;
static size_t route_count = 0;
static size_t route_capacity = 0;
static pthread_mutex_t route_mutex = PTHREAD_MUTEX_INITIALIZER;

// Define the root node for the trie-based router
static TrieNode root_node = {0}; //To be replaced later



/**
 * Initialize the libreavix router.
 * Must be called before any route registration.
 * @param max_routes Maximum routes to support
 * @return true if initialization succeeded
 *
 * This function is thread-safe and can be called from multiple threads.
 * If the router has already been initialized and max_routes is not zero,
 * this function will return false.
 */
bool reavix_init(size_t max_routes){
    if (max_routes == 0) {
        return false;
    }

    pthread_mutex_lock(&route_mutex);

    if (routes != NULL) {
        pthread_mutex_unlock(&route_mutex);
        return false;
    }

    routes = (RouteEntry*)malloc(max_routes * sizeof(RouteEntry));
    if (!routes) {
        pthread_mutex_unlock(&route_mutex);
        return false;
    }

    memset(routes, 0, max_routes * sizeof(RouteEntry));

    route_count = 0;
    route_capacity = max_routes;

    pthread_mutex_unlock(&route_mutex);
    return true;
}



/**
 * Register a new route handler for the specified HTTP method and path.
 * This function is thread-safe and ensures each route is unique.
 * 
 * @param method HTTP method (e.g., "GET", "POST") or "WS" for WebSockets.
 * @param path Route path (e.g., "/example").
 * @param handler Function to be invoked when the request matches the method and path.
 * @return true if the route was successfully registered; false if registration failed
 *         due to invalid parameters, route duplication, or capacity limitations.
 */

bool reavix_route(const char* method, const char* path, RouteHandler handler) {
    if (!method || !path || !handler) {
        return false;
    }

    pthread_mutex_lock(&route_mutex);

    if (route_count >= route_capacity) {
        pthread_mutex_unlock(&route_mutex);
        return false;
    }

    for (size_t i = 0; i < route_count; ++i) {
        if (strcmp(routes[i].method, method) == 0 && strcmp(routes[i].path, path) == 0) {
            pthread_mutex_unlock(&route_mutex);
            return false; 
        }
    }

    size_t method_len = strlen(method);
    size_t path_len = strlen(path);

    char* method_cpy = (char*)malloc(method_len + 1);
    char* path_cpy = (char*)malloc(path_len + 1);

    if (!method_cpy || !path_cpy) {
        if (method_cpy) free(method_cpy);
        if (path_cpy) free(path_cpy);
        pthread_mutex_unlock(&route_mutex);
        return false;
    }

    memcpy(method_cpy, method, method_len + 1);
    memcpy(path_cpy, path, path_len + 1);

    routes[route_count].method = method_cpy;
    routes[route_count].path = path_cpy;
    routes[route_count].handler = handler;
    route_count++;

    pthread_mutex_unlock(&route_mutex);
    return true;
}

/**
 * Dispatch a request to the first matching route handler.
 * This function is thread-safe.
 * @param method HTTP method (e.g. "GET", "POST", etc.)
 * @param path Request path (e.g. "/example")
 * @param req Immutable request object
 * @param res Mutable response object
 * @return true if a matching route handler was found and invoked; false otherwise
 */
static bool dispatch_request(const char* method, const char* path, const Request* req, Response* res) {
    char* trace_id = current_config.enable_tracing ? generate_trace_id() : NULL;
    reavix_log(LOG_DEBUG,trace_id, "Request: %s %s",method, path);

    PathParam path_params[MAX_PARAMS];
    size_t path_param_count = 0;
    RouteHandler route_handler = NULL;

    pthread_mutex_lock(&route_mutex);
    bool is_matched = trie_match(&root_node, path, path_params, &path_param_count, &route_handler);
    pthread_mutex_unlock(&route_mutex);

    if (is_matched && route_handler) {
        Request* mutable_req = (Request*)req;
        mutable_req->_internal.param_names = malloc(path_param_count * sizeof(char*));
        mutable_req->_internal.param_values = malloc(path_param_count * sizeof(char*));

        for (size_t i = 0; i < path_param_count; i++) {
            mutable_req->_internal.param_names[i] = path_params[i].name;
            mutable_req->_internal.param_values[i] = path_params[i].value;
        }

        mutable_req->_internal.param_count = path_param_count;

        route_handler(req, res);

        for (size_t i = 0; i < path_param_count; i++) {
            free(path_params[i].name);
        }
        return true;
    }

    return false;
}


/**
 * Retrieve a request header value by name.
 * Case-insensitive.
 * @param request Request object
 * @param header_key Header name
 * @return Header value if found; NULL if not found
 */
const char* req_get_header(const Request* request, const char* header_key) {
    if (!request || !header_key) {
        return NULL;
    }

    for (size_t i = 0; i < request->_internal.header_count; ++i) {
        if (strcasecmp(request->_internal.header_names[i], header_key) == 0) {
            return request->_internal.header_values[i];
        }
    }

    return NULL;
}



/**
 * Retrieve a query parameter value by name.
 * Case-insensitive.
 * @param request Request object
 * @param param_name Parameter name
 * @return Parameter value if found; NULL if not found
 */
const char* req_get_param(const Request* request, const char* param_name) {
    if (!request || !param_name) {
        return NULL;
    }

    for (size_t i = 0; i < request->_internal.param_count; ++i) {
        if (strcasecmp(request->_internal.param_names[i], param_name) == 0) {
            return request->_internal.param_values[i];
        }
    }

    return NULL;
}


/**
 * Retrieve the request body as a string.
 * @param request Request object
 * @return The request body if present; NULL if not present
 */
const char* req_get_body(const Request* request) {
    return request ? request->body : NULL;
}


/**
 * Set a header for the given response.
 * This function adds a new header name and value to the response's internal
 * storage. It allocates additional memory as needed to accommodate the new
 * header. If allocation fails, the function exits without modifying the response.
 * 
 * @param response The response object to modify.
 * @param name The header name to set.
 * @param value The header value to set.
 */

void res_set_header(Response* response, const char* name, const char* value) {
    if (!response || !name || !value) {
        return;
    }

    size_t new_count = response->_internal.header_count + 1;
    char** new_names = realloc(response->_internal.header_names, new_count * sizeof(char*));
    char** new_values = realloc(response->_internal.header_values, new_count * sizeof(char*));

    if (!new_names || !new_values) {
        free(new_names);
        free(new_values);
        return;
    }

    response->_internal.header_names = new_names;
    response->_internal.header_values = new_values;

    response->_internal.header_names[response->_internal.header_count] = strdup(name);
    response->_internal.header_values[response->_internal.header_count] = strdup(value);
    response->_internal.header_count = new_count;
}



/**
 * Sends a JSON response to the client.
 * 
 * This function sets the HTTP status code to 200, assigns the provided JSON
 * content to the response, and updates the content length accordingly. It also
 * sets the "Content-Type" header to "application/json". If the response or
 * JSON content is null, the function returns immediately without modifying
 * the response.
 * 
 * @param response Pointer to the Response object to modify.
 * @param json_body Const char pointer to the JSON content to send.
 */

void res_send_json(Response* response, const char* json_body) {
    if (!response || !json_body) {
        return;
    }

    free(response->content);

    response->status_code = 200;
    response->content = strdup(json_body);
    response->content_length = strlen(json_body);
    res_set_header(response, "Content-Type", "application/json");
}

void res_send_error(Response* res, int code, const char* message){
    if(!res || !message) return;

    res->status_code = code;
    char json[512];
    snprintf(json, sizeof(json),"{\"error\":{\"code\":%d,\"message\":\"%s\"}}",code,message);
    res_send_json(res,json);
}


/**
 * Sends a file to the client as a response.
 * 
 * This function opens the specified file, reads its contents into memory,
 * and assigns the file's contents to the response object. It sets the HTTP
 * status code to 200. It also detects the file type by its extension and
 * sets the "Content-Type" header accordingly.
 * 
 * The function returns immediately if the response or filepath is null.
 * 
 * If the file does not exist, the function sends a 404 error response.
 * If the file is too large (> 10MB), the function sends a 413 error response.
 * If the file cannot be read or memory allocation fails, the function sends
 * a 500 error response.
 * 
 * @param response Pointer to the Response object to modify.
 * @param filepath Const char pointer to the file path to read and send.
 */
void res_send_file(Response* response, const char* filepath) {
    if (!response || !filepath) {
        return;
    }

    int file_descriptor = open(filepath, O_RDONLY);
    if (file_descriptor == -1) {
        res_send_error(response, 404, "File not found");
        return;
    }

    struct stat file_stat;
    if (fstat(file_descriptor, &file_stat) == -1) {
        close(file_descriptor);
        res_send_error(response, 500, "File stat failed");
        return;
    }

    if (file_stat.st_size > 10 * 1024 * 1024) {
        close(file_descriptor);
        res_send_error(response, 413, "File too large");
        return;
    }

    char* file_content = malloc(file_stat.st_size);
    if (!file_content) {
        close(file_descriptor);
        res_send_error(response, 500, "Memory allocation failed");
        return;
    }

    ssize_t bytes_read = 0;
    while (bytes_read < file_stat.st_size) {
        ssize_t result = read(file_descriptor, file_content + bytes_read, file_stat.st_size - bytes_read);
        if (result <= 0) {
            free(file_content);
            close(file_descriptor);
            res_send_error(response, 500, "File read failed");
            return;
        }
        bytes_read += result;
    }
    close(file_descriptor);

    response->status_code = 200;
    response->content = file_content;
    response->content_length = file_stat.st_size;

    const char* file_extension = strrchr(filepath, '.');
    if (file_extension) {
        if (strcmp(file_extension, ".html") == 0) {
            res_set_header(response, "Content-Type", "text/html");
        } else if (strcmp(file_extension, ".css") == 0) {
            res_set_header(response, "Content-Type", "text/css");
        } else if (strcmp(file_extension, ".js") == 0) {
            res_set_header(response, "Content-Type", "text/javascript");
        } else {
            res_set_header(response, "Content-Type", "application/octet-stream");
        }
    } else {
        res_set_header(response, "Content-Type", "application/octet-stream");
    }
}


bool res_write(Response* res, const char* data, size_t len){
    if(!res || !data) return false;

    char* new_content = realloc(res->content, res->content_length + len +1);
    if(!new_content) return false;

    res->content = new_content;
    memcpy(res->content+res->content_length,data,len);
    res->content_length += len;
    res->content[res->content_length] = '\0';
    return true;

}


bool res_has_header(const Response* response, const char* name){
    if(!response || !name){
        return false;
    }

    for(size_t i = 0; i<response->_internal.header_count; i++){
        if(strcasecmp(response->_internal.header_names[i],name) == 0){
            return true;
        }
    }

    return false;
}


void res_remove_header(Response* response, const char* name){
    if(!response || !name) return;

    for(size_t i = 0; i < response->_internal.header_count; i++){
        if(strcasecmp(response->_internal.header_names[i],name) == 0){

            free(response->_internal.header_names[i]);
            free(response->_internal.header_values[i]);

            for(size_t j = 0; j < response->_internal.header_count - 1; j++){
                response->_internal.header_names[j] = response->_internal.header_names[j+1];
                response->_internal.header_values[j] = response->_internal.header_values[j+1];
            }

            response->_internal.header_count--;
            break;
        }
    }    
}


/**
 * Allocate and initialize a new Request object.
 *
 * The returned Request object has its internal arrays initialized to NULL, and
 * all other fields are initialized to 0 or NULL. The caller is responsible for
 * releasing the returned object with request_free().
 *
 * @return A newly allocated Request object if allocation succeeds; NULL otherwise.
 */
static Request *request_new(void) {
    Request *req = calloc(1, sizeof *req);
    if (!req) {
        return NULL;
    }

    req->_internal.header_names = NULL;
    req->_internal.header_values = NULL;
    req->_internal.param_names = NULL;
    req->_internal.param_values = NULL;

    return req;
}


/**
 * Releases all resources associated with the given Request object.
 * 
 * This function is typically invoked by the application when it is finished
 * using a Request object. The function first frees the internal arrays of
 * header names and values, as well as the internal arrays of parameter names
 * and values. It then frees the request body, if present. Finally, it frees the
 * request object itself.
 *
 * @param request The Request object to release.
 */
static void request_free(Request *request) {
    if (!request) return;

    for (size_t i = 0; i < request->_internal.header_count; ++i) {
        free((void *)request->_internal.header_names[i]);
        free((void *)request->_internal.header_values[i]);
    }

    free(request->_internal.header_names);
    free(request->_internal.header_values);

    for (size_t i = 0; i < request->_internal.param_count; ++i) {
        free((void *)request->_internal.param_names[i]);
        free((void *)request->_internal.param_values[i]);
    }

    free(request->_internal.param_names);
    free(request->_internal.param_values);

    free((void *)request->body);
    free(request);
}


void response_free(Response* response){
    if(!response) return;

    free(response->content);

    for(size_t i = 0; i <response->_internal.header_count; i++){
        free(response->_internal.header_names[i]);
        free(response->_internal.header_values[i]);
    }

    free(response->_internal.header_names);
    free(response->_internal.header_values);
}


/**
 * Allocates a new TrieNode with the given segment string.
 *
 * This function first allocates a new TrieNode structure using calloc. It then
 * duplicates the given segment string using strdup and assigns it to the
 * node->segment field. If either of these operations fail, the function returns
 * NULL. Otherwise, it returns a pointer to the newly allocated node.
 *
 * @param segment The string to be used as the segment for the new node.
 * @return A pointer to the newly allocated TrieNode, or NULL on error.
 */
static TrieNode* trie_node_new(const char* segment){
    TrieNode* node = calloc(1,sizeof(TrieNode));
    if(!node) return NULL;

    node->segment = strdup(segment);
    if(!node->segment){
        return NULL;
    }
    return node;
}


/**
 * Inserts a route handler into the trie rooted at the given root node.
 *
 * This function will allocate new nodes as necessary to represent the path
 * segments. The handler is stored in the leaf node that represents the last
 * path segment.
 *
 * @param root The root node of the trie.
 * @param path The path to be inserted.
 * @param handler The handler to be stored in the leaf node.
 * @return true if the insertion was successful; false otherwise.
 */
static bool trie_insert(TrieNode* root, const char* path, RouteHandler handler){
    if(!root || !path || !handler) return false;

    char* path_copy = strdup(path);
    if(!path_copy) return false;

    char* saveptr;
    char* segment = strtok_r(path_copy,"/",&saveptr);
    TrieNode* current = root;

    while(segment){
        bool is_param = (segment[0] == ':');
        TrieNode* target = is_param ? &current->param_child : &current->children;

        //search for existing nodes
        bool found = false;
        for(size_t i = 0; i < current->child_count; i++){
            if(strcmp(current->children[i].segment, segment) == 0){
                current = &current->children[i];
                found = true;
                break;
            }
        }

        //create new node if needed
        if(!found){
            TrieNode* new_node = trie_node_new(segment);
            if(!new_node){
                free(path_copy);
                return false;
            }

            if(is_param){
                *target = *new_node;
                free(new_node);
            } else {
                current->children = realloc(current->children, (current->child_count + 1)*sizeof(TrieNode));

                if(!current->children){
                    free(new_node->segment);
                    free(new_node);
                    free(path_copy);
                    return false;
                }
                current->children[current->child_count++] = *new_node;
                free(new_node);
            }
            current = is_param ? target : &current->children[current->child_count - 1];
        }
        segment = strtok_r(NULL,"/",&saveptr);
    }
    current->handler = handler;
    free(path_copy);
    return true;
}


/**
 * Matches a path against a Trie rooted at the given node.
 *
 * This function returns true if the path matches a route in the Trie, and
 * false otherwise. If a match is found, the handler associated with the
 * matching route is stored in the location pointed to by out_handler.
 * Additionally, any path parameters (segments starting with ':') are stored
 * in the array pointed to by params, and the number of parameters is stored
 * in the location pointed to by param_count.
 *
 * @param root The root node of the Trie.
 * @param path The path to be matched.
 * @param params An array to store the path parameters.
 * @param param_count A pointer to store the number of path parameters.
 * @param out_handler A pointer to store the handler associated with the
 *                    matching route.
 * @return true if a match was found; false otherwise.
 */
static bool trie_match(TrieNode* root, const char* path, PathParam params[], size_t* param_count, RouteHandler* out_handler) {
    if (!root || !path || !out_handler) {
        return false;
    }

    char* path_copy = strdup(path);
    if (!path_copy) {
        return false;
    }

    char* saveptr = NULL;
    char* segment = strtok_r(path_copy, "/", &saveptr);
    TrieNode* current = root;
    size_t param_index = 0;

    while (segment) {
        bool found = false;
        for (size_t i = 0; i < current->child_count; i++) {
            if (strcmp(current->children[i].segment, segment) == 0) {
                current = &current->children[i];
                found = true;
                break;
            }
        }

        if (!found && current->param_child) {
            if (param_index < MAX_PARAMS) {
                params[param_index].name = current->param_child->segment + 1;
                params[param_index].value = strdup(segment);
                param_index++;
            }
            current = current->param_child;
            found = true;
        }

        if (!found) {
            free(path_copy);
            return false;
        }

        segment = strtok_r(NULL, "/", &saveptr);
    }

    *out_handler = current->handler;
    *param_count = param_index;
    free(path_copy);
    return true;
}



//Logging functions
// Can be adjusted later
#include <time.h>
#include <stdarg.h>

static LogConfig current_config = {
    .min_level = LOG_INFO,
    .enable_tracing = true,
    .colored_output = true,
    .custom_handler = NULL
};

//I think ANSI color codes are the best for this
static const char* Level_colors[] = {
    "\x1b[36m", //TRACE (cyan)
    "\x1b[34m", //DEBUG (blue)
    "\x1b[32m", //INFO (green)
    "\x1b[33m", //WARNING (yellow)
    "\x1b[31m", //ERROR (red)
    "\x1b[35m"  //FATAL (magenta)

};

void reavix_log(LogLevel level, const char* trace_id, const char* format, ...){
    if(level < current_config.min_level) return;

    char message[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    //Use custom handler if provided
    if(current_config.custom_handler){
        current_config.custom_handler(level, trace_id, message);
        return;
    }

    //Default console ouput
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    const char* level_str = "";
    switch(level){
        case LOG_TRACE: level_str = "TRACE"; break;
        case LOG_DEBUG: level_str = "DEBUG"; break;
        case LOG_INFO: level_str = "INFO"; break;
        case LOG_WARNING: level_str = "WARNING"; break;
        case LOG_ERROR: level_str = "ERROR"; break;
        case LOG_FATAL: level_str = "FATAL"; break;
    }
    if(current_config.colored_output){
        fprintf(stderr, "%s %s%-5s\x1b[0m \x1b[90m%s\x1b[0m %s\n", timestamp,Level_colors[level], level_str, trace_id ? trace_id : "-", message);
    }else{
        fprintf(stderr, "%s %-5s %s %s\n", timestamp, level_str,trace_id ? trace_id : "-", message);
    }
}


/**
 * Generates a UUID-like trace ID. This is used to uniquely identify requests
 * as they pass through the system.
 *
 * The generated ID is a 36-character string in the format:
 * XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX
 *
 * @return A pointer to the generated trace ID. This memory must be freed by
 *         the caller when it is no longer needed.
 */
static char* generate_trace_id(void) {
    const char charset[] = "0123456789abcdef";
    char* id = malloc(37);
    if (!id) {
        return NULL;
    }

    // Generate the trace ID
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            // Hyphens at the standard UUID positions
            id[i] = '-';
        } else {
            // Random characters from the charset
            id[i] = charset[rand() % (sizeof(charset) - 1)];
        }
    }

    // Null-terminate the string
    id[36] = '\0';
    return id;
}

#include <sys/time.h>

void log_metrics(const Request* req){
    RequestMetrics* metrics = (RequestMetrics*)req->_internal.metrics;
    if(!metrics) return;

    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    double latency = (end_time.tv_sec - metrics->start_time.tv_sec) * 1000.0;
    latency += (end_time.tv_sec - metrics->start_time.tv_sec) / 1000.0;

    reavix_log(LOG_INFO,req->_internal.trace_id, "Metrics: %.2fms | %zuKB | Req#%",latency,metrics->memory_usage/1024, metrics->requests_handled);
}

/**
 * Configures the logging system with a new log configuration.
 * 
 * This function updates the current logging configuration, which affects
 * how logging messages are processed and displayed. The configuration
 * includes settings such as minimum log level, whether tracing is enabled,
 * whether to use colored output, and any custom log handler.
 *
 * @param config The new log configuration to apply.
 */

void reavix_log_configure(LogConfig config){
    current_config = config;
}
