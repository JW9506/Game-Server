#include <iostream>
#include <cstdlib>
#include <uv.h>
#include "netbus.h"
#include "uv_session.h"
#include "ws_protocol.h"
#include "tp_protocol.h"
#include "proto_man.h"
#include "service_man.h"

#ifdef __cplusplus
extern "C" {
#endif
static void on_recv_client_cmd(uv_session* s, unsigned char* body, int len) {
    struct cmd_msg* msg = NULL;
    if (proto_man::decode_cmd_msg(body, len, &msg)) {
        if (!service_man::on_recv_cmd_msg((session*)s, msg)) { s->close(); }
        proto_man::cmd_msg_free(msg);
    }
}

static void on_recv_tcp_data(uv_session* s) {
    unsigned char* pkg_data =
        (unsigned char*)(s->long_pkg != NULL ? s->long_pkg : s->recv_buf);
    while (s->recved > 0) {
        int pkg_size = 0;
        int head_size = 0;

        if (!tp_protocol::read_header(pkg_data, s->recved, &pkg_size,
                                      &head_size)) {
            break;
        }
        if (s->recved < pkg_size) { break; }
        unsigned char* raw_data = pkg_data + head_size;

        on_recv_client_cmd(s, raw_data, pkg_size - head_size);
        if (s->recved > pkg_size) {
            // remove handled pkg (by moving it to the lower mem space)
            memmove(pkg_data, pkg_data + pkg_size, s->recved - pkg_size);
        }
        s->recved -= pkg_size;
        if (s->recved == 0 && s->long_pkg != NULL) {
            free(s->long_pkg);
            s->long_pkg = NULL;
            s->long_pkg_size = 0;
        }
    }
}

static void on_recv_ws_data(uv_session* s) {
    unsigned char* pkg_data =
        (unsigned char*)(s->long_pkg != NULL ? s->long_pkg : s->recv_buf);
    while (s->recved > 0) {
        int pkg_size = 0;
        int head_size = 0;

        if (pkg_data[0] == 0x88) {
            s->close();
#ifdef _DEBUG
            printf("ws closing\n");
#endif
            break;
        }
        if (!ws_protocol::read_ws_header(pkg_data, s->recved, &pkg_size,
                                         &head_size)) {
            break;
        }
        if (s->recved < pkg_size) { break; }
        unsigned char* raw_data = pkg_data + head_size;
        unsigned char* mask = pkg_data + head_size - 4;
        ws_protocol::parse_ws_recv_data(raw_data, mask, pkg_size - head_size);
        on_recv_client_cmd(s, raw_data, pkg_size - head_size);
        if (s->recved > pkg_size) {
            // remove handled pkg (by moving it to the lower mem space)
            memmove(pkg_data, pkg_data + pkg_size, s->recved - pkg_size);
        }
        s->recved -= pkg_size;
        if (s->recved == 0 && s->long_pkg != NULL) {
            free(s->long_pkg);
            s->long_pkg = NULL;
            s->long_pkg_size = 0;
        }
    }
}

static void alloc_buf(uv_handle_t* handle, size_t suggested_size,
                      uv_buf_t* buf) {
    uv_session* s = (uv_session*)handle->data;
    if (s->recved < RECV_LEN) {
        *buf = uv_buf_init(s->recv_buf + s->recved, RECV_LEN - s->recved);
    } else {
        if (s->long_pkg == NULL) {
            if (s->socket_type == WS_SOCKET) {
                if (s->socket_type == WS_SOCKET && s->did_shake_hand) {
                    int pkg_size;
                    int head_size;
                    ws_protocol::read_ws_header((unsigned char*)s->recv_buf,
                                                s->recved, &pkg_size,
                                                &head_size);
                    s->long_pkg_size = pkg_size;
                    s->long_pkg = (char*)malloc(pkg_size);
                    memcpy(s->long_pkg, s->recv_buf, s->recved);
                } else {
                    // tcp > RECV_LEN
                }
            }
        } else {
            *buf = uv_buf_init(s->long_pkg + s->recved,
                               s->long_pkg_size - s->recved);
        }
    }
}
static void after_read(uv_stream_t* stream, ssize_t nread,
                       const uv_buf_t* buf) {
    uv_session* s = (uv_session*)stream->data;
    if (nread < 0) {
        s->close();
        return;
    }
    s->recved += (int)nread;
    if (s->socket_type == WS_SOCKET) {
        if (s->did_shake_hand == 0) {
            if (ws_protocol::ws_shake_hand((session*)s, s->recv_buf,
                                           s->recved)) {
                s->did_shake_hand = 1;
                s->recved = 0;
            }
        } else {
            on_recv_ws_data(s);
        }
    } else {
        on_recv_tcp_data(s);
    }
}
static void on_connection(uv_stream_t* server, int status) {
    uv_session* s = uv_session::create();
    uv_tcp_t* client = &s->tcp_handle;
    uv_tcp_init(uv_default_loop(), client);
    client->data = (void*)s;
    uv_accept(server, (uv_stream_t*)client);

    struct sockaddr_in addr;
    int len = sizeof(addr);
    uv_tcp_getpeername((const uv_tcp_t*)client, (sockaddr*)&addr, &len);
    uv_ip4_name(&addr, s->c_address, sizeof(s->c_address));
    s->c_port = ntohs(addr.sin_port);
    s->socket_type = (int)((size_t)server->data);
    printf("New client: %s:%d\n", s->c_address, s->c_port);

    uv_read_start((uv_stream_t*)client, alloc_buf, after_read);
}
#ifdef __cplusplus
}
#endif

static netbus* g_netbus;
netbus* netbus::instance() {
    if (!g_netbus) {
        g_netbus = new netbus;
        g_netbus->init();
    }
    return g_netbus;
}

void netbus::start_tcp_server(int port) {
    uv_tcp_t* listen = static_cast<uv_tcp_t*>(malloc(sizeof(uv_tcp_t)));
    memset(listen, 0, sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), listen);

    struct sockaddr_in addr;
    char* host{ "127.0.0.1" };
    uv_ip4_addr(host, port, &addr);
    if (uv_tcp_bind(listen, (struct sockaddr*)&addr, 0) != 0) {
        printf("bind error\n");
        free(listen);
        return;
    }
    printf("Listening %s:%d\n", host, port);
    uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connection);
    listen->data = (void*)TCP_SOCKET;
}

void netbus::start_ws_server(int port) {
    uv_tcp_t* listen = static_cast<uv_tcp_t*>(malloc(sizeof(uv_tcp_t)));
    memset(listen, 0, sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop(), listen);

    struct sockaddr_in addr;
    char* host{ "127.0.0.1" };
    uv_ip4_addr(host, port, &addr);
    if (uv_tcp_bind(listen, (struct sockaddr*)&addr, 0) != 0) {
        printf("bind error\n");
        free(listen);
        return;
    }
    printf("Listening ws://%s:%d\n", host, port);
    uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connection);
    listen->data = (void*)WS_SOCKET;
}

void netbus::run() { uv_run(uv_default_loop(), UV_RUN_DEFAULT); }

void netbus::init() {
    service_man::init();
    init_session_allocer();
}
