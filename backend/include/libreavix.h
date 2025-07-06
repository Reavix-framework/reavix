#ifndef LIBREAVIX_H
#define LIBREAVIX_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <uv.h>


typedef struct Request {
    const char* method;
    const char* path;
    const char* query;
    const char* body;

    struct{
        const char** header_names;
        const char** header_values;
        size_t header_count;

        const char** param_names;
        const char** param_values;
        size_t param_count;
        RequestMetrics* metrics;
        char* trace_id;
    }_internal;
};

typedef struct Response{
    int status_code;
    char* content;
    size_t content_length;

    struct{
        const char** header_names;
        const char** header_values;
        size_t header_count;
        uv_stream_t* client;
        RequestMetrics* metrics;
    } _internal;
} Response;

//Forward declarations
typedef struct Request Request;
typedef struct Response Response;

typedef enum{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

//Log callback signature
typedef void (*LogHandler)(LogLevel level, const char* message, const char* trace_id);

//configuration structure
typedef struct{
    LogLevel min_level;
    bool enable_tracing;
    bool colored_output;
    LogHandler custom_handler;
} LogConfig;

typedef struct{
    struct timeval start_time;
    size_t memory_usage;
    uint64_t requests_handled;
} RequestMetrics;

/**
 * Route handler signature
 * @param req Immutable request object
 * @param res Mutable response object
 */
typedef void (*RouteHandler)(const Request* req, Response* res);


typedef struct{
    char* name;
    char* value;
}PathParam;

typedef struct TrieNode{
    char* segment;
    RouteHandler handler;
    struct TrieNode* children;
    struct TrieNode* param_child;
    size_t child_count;
} TrieNode;


/**
 * Initialize the libreavix router.
 * Must be called before any route registration.
 * @param max_routes Maximum routes to support
 * @return true if initialization succeeded
 */
bool reavix_init(size_t max_routes);


/**
 * Register a route handler
 * @param method HTTP method (e.g. "GET", "POST", etc.) or "WS" for WebSockets
 * @param path Route path (e.g, "/read-file")
 * @param handler Function to call when request matches
 * @return true if route registration succeeded
 */
bool reavix_route(const char* method, const char* path, RouteHandler handler);


/**
 * Starts the Reavix server on the specified port.
 * This function listens for incoming connections and handles requests
 * using the registered route handlers.
 * @param port The port number on which the server will listen for requests.
 */

void reavix_server(uint16_t port);

//Response helpers
void res_send_json(Response* res, const char* json);
void res_send_file(Response* res, const char* filepath);
void res_set_header(Response* res, const char* key, const char* value);
void res_send_error(Response* res, int code, const char* message);
bool res_write(Response* res, const char* data, size_t len);

//Request helpers
const char* req_get_header(const Request* req, const char* key);
const char* req_get_param(const Request* req, const char* key);
const char* req_get_body(const Request* req);

//error handler
typedef void(*ErrorHandler)(int code, const char* message);
void reavix_set_error_handler(ErrorHandler handler);

//header utilities
bool res_has_header(const Response* res, const char* key);
void res_remove_header(Response* res, const char* key);


#endif