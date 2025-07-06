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
    pthread_mutex_lock(&route_mutex);

    for (size_t i = 0; i < route_count; i++) {
        if (strcmp(routes[i].method, method) == 0 && strcmp(routes[i].path, path) == 0) {
            RouteHandler route_handler = routes[i].handler;
            pthread_mutex_unlock(&route_mutex);
            route_handler(req, res);
            return true;
        }
    }

    pthread_mutex_unlock(&route_mutex);
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


static TrieNode* trie_node_new(const char* segment){
    TrieNode* node = calloc(1,sizeof(TrieNode));
    if(!node) return NULL;

    node->segment = strdup(segment);
    if(!node->segment){
        return NULL;
    }
    return node;
}


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
                *target = new_node;
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
            current = is_param ? *target : &current->children[current->child_count - 1];
        }
        segment = strtok_r(NULL,"/",&saveptr);
    }
    current->handler = handler;
    free(path_copy);
    return true;
}


static bool trie_match(TrieNode* root, const char* path, PathParam* params, size_t* param_count, RouteHandler* out_handler){
    if(!root || !path || !out_handler) return false;

    char* path_copy = strdup(path);
    if(!path_copy)return false;

    char* saveptr;
    char* segment = strtok_r(path_copy,"/",&saveptr);
    TrieNode* current = root;

    while(segment){
        //We try exact match first
        bool found = false;
        for(size_t i = 0; i < current->child_count; i++){
            if(strcmp(current->children[i].segment,segment)==0){
                current = &current->children[i];
                found = true;
                break;
            }
        }
        //fall back to parameter match
        if(!found && current->param_child){
            if(*param_count < MAX_PARAMS){
                params[*param_count].name = current->param_child->segment + 1;
                params[*param_count].value = strdup(segment);
                (*param_count)++;
            }
            current = current->param_child;
            found = true;
        }

        if(!found){
            free(path_copy);
            return false;
        }

        segment = strtok_r(NULL,"/",&saveptr);
    }
    *out_handler = current->handler;
    free(path_copy);
    return true;
}
