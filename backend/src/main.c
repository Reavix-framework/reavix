#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include "router.h"

uv_loop_t* loop;

void on_connection(uv_stream_t* server, int status){
    if(status < 0){
        fprintf(stderr, "Connection error: %s\n", uv_strerror(status));
        return;
    }

    client_t* client = malloc(sizeof(client_t));
    uv_tcp_init(loop, &client->handle);

    if(uv_accept(server, (uv_stream_t*)&client->handle) == 0){
        uv_read_start((uv_stream_t*)&client->handle, on_alloc, on_read);
    }else{
        uv_close((uv_handle_t*)&client->handle, on_close);
    }
    
}

int main(){
    loop = uv_default_loop();

    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0",HTTP_PORT, &addr);

    uv_tcp_bind(&server, (const struct sockaddr*)&addr,0);
    int r = uv_listen((uv_stream_t*)&server, 128, on_connection);

    if(r){
        fprintf(stderr, "Listen error: %s\n", uv_strerror(r));
        return 1;
    }

    printf("Server running at http://localhost:%d\n", HTTP_PORT);
    return uv_run(loop, UV_RUN_DEFAULT);
}