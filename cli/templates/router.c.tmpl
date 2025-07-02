#include <uv.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "router.h"


static const char* http_status_message(int status) {
    switch (status) {
        case 200: return "OK";
        case 404: return "Not Found";
        case 400: return "Bad Request";
        case 500: return "Internal Server Error";
        default:  return "";
    }
}



void after_write(uv_write_t* req, int status) {
    if (req->data) free(req->data); 
    free(req); 
}


void send_response(uv_stream_t* client, const char* content, const char* content_type, int status) {
    const char* status_msg = http_status_message(status);
    size_t content_len = strlen(content);
    
    int header_len = snprintf(NULL, 0,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status, status_msg, content_type, content_len);
    size_t total_len = header_len + content_len;
    char* response = malloc(total_len + 1);
    if (!response) return;

    snprintf(response, total_len + 1,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        status, status_msg, content_type, content_len, content);

    uv_buf_t buf = uv_buf_init(response, total_len);

    uv_write_t* write_req = malloc(sizeof(uv_write_t));
    if (!write_req) {
        free(response);
        return;
    }
    write_req->data = response; 

    uv_write(write_req, client, &buf, 1, after_write);
}


void route_request(uv_stream_t* client, const char* method, const char* path) {
    if (strcmp(path, "/") == 0) {
        send_response(client, "<h1>Reavix Backend </h1>", "text/html", 200);
    }else if(strcmp(path, "/api/health") == 0){
        send_response(client, "OK", "text/plain", 200);
        return;
    }
    else {
        send_response(client, "<h1>Not Found</h1>", "text/html", 404);
    }
}


void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if (nread <= 0) {
        if (buf->base) free(buf->base);
        uv_close((uv_handle_t*)stream, on_close);
        return;
    }

    
    buf->base[nread < buf->len ? nread : buf->len - 1] = '\0';

    char* method = strtok(buf->base, " ");
    char* path = strtok(NULL, " ");

    if (method && path) {
        route_request(stream, method, path);
    } else {
        send_response(stream, "<h1>Bad Request</h1>", "text/html", 400);
    }

    free(buf->base);
    uv_close((uv_handle_t*)stream, on_close);
}


void on_alloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}


void on_close(uv_handle_t* handle) {
    
    client_t* client = (client_t*)handle;
    free(client);
}