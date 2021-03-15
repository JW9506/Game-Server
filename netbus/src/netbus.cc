#include "netbus.h"
#include <iostream>
#include <cstdlib>
#include <uv.h>

#ifdef __cpluslus
extern "C" {
#endif

#define _new(type, var) type* var = (type*)calloc(1, sizeof(type))
static void alloc_cb(uv_handle_t* handle, size_t suggested_size,
                     uv_buf_t* buf) {
    if (handle->data) {
        free(handle->data);
        handle->data = NULL;
    }
    buf->base = (char*)malloc(suggested_size + 1);
    buf->len = (ULONG)suggested_size;
    handle->data = buf->base;
}
static void close_cb(uv_handle_t* handle) {
    printf("a client disconnected\n");
    if (handle->data) {
        free(handle->data);
        handle->data = NULL;
    }
    free(handle);
}
static void shutdown_cb(uv_shutdown_t* req, int status) {
    uv_close((uv_handle_t*)req->handle, close_cb);
    free(req);
}
static void write_cb(uv_write_t* req, int status) {
    if (status == 0) { printf("write success\n"); }
    if (req->data) { free(req->data); }
    free(req);
}
static void send_data(uv_stream_t* stream, unsigned char* send_data,
                      size_t send_len) {
    _new(uv_write_t, w_req);
    _new(uv_buf_t, w_buf);
    char* send_buf = (char*)malloc(send_len);
    memcpy_s(send_buf, send_len, send_data, send_len);
    w_buf->base = send_buf;
    w_buf->len = (ULONG)send_len;
    w_req->data = w_buf;
    uv_write(w_req, stream, w_buf, 1, write_cb);
}
static void after_read(uv_stream_t* stream, ssize_t nread,
                       const uv_buf_t* buf) {
    if (nread < 0) {
        _new(uv_shutdown_t, req);
        uv_shutdown(req, stream, shutdown_cb);
        return;
    }
    buf->base[nread] = 0;
    printf("recv %d\n", (int)nread);
    printf("%s\n", buf->base);
}
static void on_connection(uv_stream_t* server, int status) {
    printf("new client\n");
    _new(uv_tcp_t, client);
    uv_tcp_init(uv_default_loop(), client);
    uv_accept(server, (uv_stream_t*)client);

    uv_read_start((uv_stream_t*)client, alloc_cb, after_read);
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
