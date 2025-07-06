#ifndef ROUTER_H
#define ROUTER_H

#include <uv.h>
#include "libreavix.h"

#define HTTP_PORT 8081
#define STATIC_DIR "static"

typedef struct {
    uv_tcp_t handle;
    Request req;
    Response res;
    char* trace_id;
}clientContext;



void on_alloc(uv_handle_t* handle, size_t suggested_seze, uv_buf_t* buf);
void on_read(uv_stream_t* client, ssize_t nread, const uv_buf_t* buf);
void on_close(uv_handle_t* handle);
void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void on_new_connection(uv_stream_t* server, int status);


#endif