#include "libreavix.h";
#include "router.h";
#include <uv.h>

uv_loop_t* loop;
TrieNode route_tree;



int main(){

    //Initialize libreavix
    if(!reavix_init(100)){
        REAVIX_LOG(LOG_FATAL,NULL,"Router initialization failed");
        return 1;
    }

    //Register routes
    reavix_route("GET","/api/users",handle_get_users);
    reavix_route("POST","/api/upload",handle_upload);

    //start libuv event loop
    loop = uv_default_loop();
    uv_tcp_t server;
    uv_tcp_init(loop, &server);

    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 8081, &addr);
    uv_tcp_bind(&server, (const struct sockaddr*)&addr, 0);

    uv_listen((uv_stream_t*)&server, 128, on_new_connection);
    REAVIX_LOG(LOG_INFO,NULL,"Server running on port 8081");

    return uv_run(loop, UV_RUN_DEFAULT);
}