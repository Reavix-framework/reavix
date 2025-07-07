#ifndef LIBREAVIX_H
#define LIBREAVIX_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <uv.h>

// Protocol definitions
#define PROTOCOL_HTTP 0x01
#define PROTOCOL_WS   0x02  // WebSocket
#define PROTOCOL_IPC  0x04  // Inter-process communication
#define PROTOCOL_QUIC 0x08  // QUIC/UDP support

// Compression types
typedef enum {
    COMPRESSION_NONE = 0,
    COMPRESSION_GZIP,
    COMPRESSION_BROTLI,
    COMPRESSION_DEFLATE
} CompressionType;

// Rate limiting configuration
typedef struct {
    uint32_t requests_per_minute;
    uint32_t burst_limit;
    bool enabled;
} RateLimitConfig;

// Security policies
typedef struct {
    bool cors_enabled;
    const char** cors_origins;
    bool csrf_protection;
    bool content_security_policy;
    bool hsts_enabled;
} SecurityPolicy;

typedef struct Request {
    const char* method;
    const char* path;
    const char* query;
    const char* body;
    size_t body_length;
    uint8_t protocol;

    struct {
        const char** header_names;
        const char** header_values;
        size_t header_count;

        const char** param_names;
        const char** param_values;
        size_t param_count;
        
        void* metrics;
        char* trace_id;
        void* plugin_data;
        CompressionType compression;
    } _internal;
} Request;

typedef struct Response {
    int status_code;
    char* content;
    size_t content_length;
    uint8_t protocol;

    struct {
        char** header_names;
        char** header_values;
        size_t header_count;
        uv_stream_t* client;
        void* metrics;
        bool headers_sent;
        CompressionType compression;
    } _internal;
} Response;

typedef enum {
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} LogLevel;

typedef void (*LogHandler)(LogLevel level, const char* message, const char* trace_id);
typedef void (*RouteHandler)(const Request* req, Response* res);
typedef void (*ErrorHandler)(int code, const char* message);
typedef void (*PluginInit)(void);
typedef void (*PluginRequestHook)(Request* req, Response* res);
typedef void (*Middleware)(Request* req, Response* res, void (*next)(Request*, Response*));

typedef struct {
    LogLevel min_level;
    bool enable_tracing;
    bool colored_output;
    LogHandler custom_handler;
} LogConfig;

typedef struct {
    struct timeval start_time;
    size_t memory_usage;
    uint64_t requests_handled;
    uint64_t bytes_sent;
} RequestMetrics;

typedef struct {
    char* name;
    char* value;
} PathParam;

typedef struct TrieNode {
    char* segment;
    RouteHandler handler;
    struct TrieNode* children;
    struct TrieNode* param_child;
    size_t child_count;
    RateLimitConfig rate_limit;
} TrieNode;

typedef struct {
    const char* name;
    PluginInit init;
    PluginRequestHook pre_handler;
    PluginRequestHook post_handler;
} Plugin;

// Core API
bool reavix_init(size_t max_routes);
bool reavix_route(const char* method, const char* path, RouteHandler handler);
void reavix_server(uint16_t port);

// Response helpers
void res_send_json(Response* res, const char* json);
void res_send_file(Response* res, const char* filepath);
void res_set_header(Response* res, const char* key, const char* value);
void res_send_error(Response* res, int code, const char* message);
bool res_write(Response* res, const char* data, size_t len);
void res_compress(Response* res, CompressionType type);

// Request helpers
const char* req_get_header(const Request* req, const char* key);
const char* req_get_param(const Request* req, const char* key);
const char* req_get_body(const Request* req);
size_t req_get_body_length(const Request* req);

// Header utilities
bool res_has_header(const Response* res, const char* key);
void res_remove_header(Response* res, const char* key);

// Configuration
void reavix_set_error_handler(ErrorHandler handler);
void reavix_log_configure(LogConfig config);
void reavix_set_security_policy(SecurityPolicy policy);
void reavix_set_rate_limits(const char* path, RateLimitConfig config);

// Plugin system
void reavix_register_plugin(Plugin plugin);
void reavix_enable_protocol(uint8_t protocol);

// Middleware
void reavix_use(Middleware middleware);

// WebSocket support
void reavix_ws_send(Response* res, const char* message);
void reavix_ws_broadcast(const char* message);

#endif // LIBREAVIX_H