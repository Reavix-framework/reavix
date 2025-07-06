#include "libreavix.h"
#include "router.h"

extern uv_loop_t* loop;


/**
 * @brief Callback for new incoming connections
 * @param server The server instance that triggered this callback
 * @param status The status of the new connection
 *
 * This callback is called whenever a new connection is established.
 * It is responsible for initializing the client context and starting the
 * read loop for the connection.
 *
 * If an error occurs during this process, the connection is closed and
 * an error message is logged.
 */
void on_new_connection(uv_stream_t* server, int status) {
    if (status < 0) {
        REAVIX_LOG(LOG_ERROR, NULL, "Connection error: %s", uv_strerror(status));
        return;
    }

    clientContext* client_ctx = malloc(sizeof(clientContext));
    if (!client_ctx) {
        REAVIX_LOG(LOG_ERROR, NULL, "Memory allocation failed for client context");
        return;
    }

    client_ctx->trace_id = malloc(16);
    if (!client_ctx->trace_id) {
        free(client_ctx);
        REAVIX_LOG(LOG_ERROR, NULL, "Memory allocation failed for trace ID");
        return;
    }

    uv_tcp_init(loop, &client_ctx->handle);
    if (uv_accept(server, (uv_stream_t*)&client_ctx->handle) < 0) {
        free(client_ctx->trace_id);
        free(client_ctx);
        REAVIX_LOG(LOG_ERROR, NULL, "Failed to accept connection");
        return;
    }

    memset(&client_ctx->req, 0, sizeof(Request));
    memset(&client_ctx->res, 0, sizeof(Response));

    uv_read_start((uv_stream_t*)&client_ctx->handle, alloc_buffer, on_read);
}
