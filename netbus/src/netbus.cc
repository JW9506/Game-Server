#include <iostream>
#include <cstdlib>
#include <uv.h>
#include "netbus.h"
#include "uv_session.h"

#ifdef __cpluslus
extern "C" {
#endif

static void alloc_buf(uv_handle_t* handle, size_t suggested_size,
                      uv_buf_t* buf) {
    uv_session* s = (uv_session*)handle->data;
    *buf = uv_buf_init(s->recv_buf + s->recved, RECV_LEN - s->recved);
}
static void after_read(uv_stream_t* stream, ssize_t nread,
                       const uv_buf_t* buf) {
    uv_session* s = (uv_session*)stream->data;
    if (nread < 0) {
        s->close();
        return;
    }
    buf->base[nread] = 0;
    printf("recv %d\n", (int)nread);
    printf("%s\n", buf->base);
    s->send_data("hello", strlen("hello"));
}
static void on_connection(uv_stream_t* server, int status) {
    uv_session* s = uv_session::create();
    uv_tcp_t* client = &s->tcp_handle;
    uv_tcp_init(uv_default_loop(), client);
    client->data = (void*)s;
    uv_accept(server, (uv_stream_t*)client);

    SOCKADDR_IN addr;
    int len = sizeof(addr);
    uv_tcp_getpeername((const uv_tcp_t*)client, (sockaddr*)&addr, &len);
    uv_ip4_name(&addr, s->c_address, sizeof(s->c_address));
    s->c_port = ntohs(addr.sin_port);
    printf("New client: %s:%d\n", s->c_address, s->c_port);

    uv_read_start((uv_stream_t*)client, alloc_buf, after_read);
}
#ifdef __cpluslus
}
#endif

static netbus* g_netbus;
netbus* netbus::instance() {
    if (!g_netbus) { g_netbus = new netbus; }
    return g_netbus;
}

void netbus::start_tcp_server(int port) {
    uv_tcp_t* listen = static_cast<uv_tcp_t*>(malloc(sizeof(uv_tcp_t)));
    memset(listen, 0, sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), listen);

    SOCKADDR_IN addr;
    char* host{ "127.0.0.1" };
    uv_ip4_addr(host, port, &addr);
    int ret = uv_tcp_bind(listen, (SOCKADDR*)&addr, 0);
    if (ret != 0) {
        printf("bind error\n");
        free(listen);
        return;
    }
    printf("Listening %s:%d\n", host, port);
    uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connection);
}
void netbus::start_ws_server(int port) { }

void netbus::run() { uv_run(uv_default_loop(), UV_RUN_DEFAULT); }