#include <iostream>
#include <cstdlib>
#include <uv.h>
#include "netbus.h"
#include "uv_session.h"
#include "udp_session.h"
#include "session.h"
#include "ws_protocol.h"
#include "tp_protocol.h"
#include "proto_man.h"
#include "service_man.h"

#ifdef __cplusplus
extern "C" {
#endif
static void on_recv_client_cmd(session* s, unsigned char* body, int len) {
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

        on_recv_client_cmd((session*)s, raw_data, pkg_size - head_size);
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
        on_recv_client_cmd((session*)s, raw_data, pkg_size - head_size);
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
            if (s->socket_type == WS_SOCKET && s->did_shake_hand) {
                int pkg_size;
                int head_size;
                ws_protocol::read_ws_header((unsigned char*)s->recv_buf,
                                            s->recved, &pkg_size, &head_size);
                s->long_pkg_size = pkg_size;
                s->long_pkg = (char*)malloc(pkg_size);
                memcpy(s->long_pkg, s->recv_buf, s->recved);
            } else {
                // tcp > RECV_LEN
                int pkg_size;
                int head_size;
                tp_protocol::read_header((unsigned char*)s->recv_buf, s->recved,
                                         &pkg_size, &head_size);
                s->long_pkg_size = pkg_size;
                s->long_pkg = (char*)malloc(pkg_size);
                memcpy(s->long_pkg, s->recv_buf, s->recved);
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

struct udp_recv_buf {
    char* recv_buf;
    int max_recv_len;
};
static void udp_uv_alloc_cb(uv_handle_t* handle, size_t suggested_size,
                            uv_buf_t* buf) {
    suggested_size = suggested_size > 8096 ? suggested_size : 8096;
    struct udp_recv_buf* udp_buf = (struct udp_recv_buf*)handle->data;
    if (udp_buf->max_recv_len < suggested_size) {
        if (udp_buf->recv_buf) {
            free(udp_buf->recv_buf);
            udp_buf->recv_buf = NULL;
        }
        udp_buf->recv_buf = (char*)malloc(suggested_size);
        udp_buf->max_recv_len = (int)suggested_size;
    }
    buf->base = udp_buf->recv_buf;
    buf->len = (ULONG)suggested_size;
}

static void after_udp_recv_cb(uv_udp_t* handle, ssize_t nread,
                              const uv_buf_t* buf, const struct sockaddr* addr,
                              unsigned flags) {
    udp_session udp_s;
    udp_s.udp_handle = *handle;
    udp_s.addr = addr;
    udp_s.c_port = ntohs(((struct sockaddr_in*)addr)->sin_port);
    uv_ip4_name((struct sockaddr_in*)addr, udp_s.c_address,
                sizeof(udp_s.c_address));
    on_recv_client_cmd((session*)&udp_s, (unsigned char*)buf->base, (int)nread);
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

void netbus::tcp_server(int port) {
    uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
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

void netbus::udp_server(int port) {
    uv_udp_t* server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
    memset(server, 0, sizeof(uv_udp_t));

    uv_udp_init(uv_default_loop(), server);
    struct udp_recv_buf* buf =
        (struct udp_recv_buf*)malloc(sizeof(struct udp_recv_buf));
    memset(buf, 0, sizeof(struct udp_recv_buf));
    server->data = (void*)buf;

    struct sockaddr_in addr;
    char* host{ "127.0.0.1" };
    uv_ip4_addr(host, port, &addr);
    if (uv_udp_bind(server, (struct sockaddr*)&addr, 0) != 0) {
        printf("bind error\n");
        free(server);
        return;
    }
    printf("Listening(UDP)... %s:%d\n", host, port);
    uv_udp_recv_start(server, udp_uv_alloc_cb, after_udp_recv_cb);
}

void netbus::ws_server(int port) {
    uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
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

struct connect_cb {
    void (*on_connected)(int err, session* s, void* udata);
    void* udata;
};
void after_connect(uv_connect_t* req, int status) {
    uv_session* s = (uv_session*)req->handle->data;
    struct connect_cb* cb = (struct connect_cb*)req->data;
    if (status) {
        if (cb->on_connected) { cb->on_connected(1, NULL, cb->udata); }
        s->close();
        free(cb);
        free(req);
        return;
    }
    if (cb->on_connected) { cb->on_connected(0, (session*)s, cb->udata); }
    uv_read_start((uv_stream_t*)req->handle, alloc_buf, after_read);
    free(cb);
    free(req);
}

void netbus::tcp_connect(const char* server_ip, int port,
                         void (*on_connected)(int err, session* s, void* udata),
                         void* udata) {
    struct sockaddr_in bind_addr;
    if (uv_ip4_addr(server_ip, port, &bind_addr)) { return; }

    uv_session* s = uv_session::create();
    uv_tcp_t* client = &s->tcp_handle;
    uv_tcp_init(uv_default_loop(), client);
    client->data = (void*)s;
    s->as_client = 1;

    s->socket_type = TCP_SOCKET;
    strcpy(s->c_address, server_ip);
    s->c_port = port;
    uv_connect_t* connect_req = (uv_connect_t*)malloc(sizeof(uv_connect_t));
    struct connect_cb* cb =
        (struct connect_cb*)malloc(sizeof(struct connect_cb));
    cb->on_connected = on_connected;
    cb->udata = udata;
    connect_req->data = (void*)cb;
    if (uv_tcp_connect(connect_req, client, (struct sockaddr*)&bind_addr,
                       after_connect)) {
        return;
    }
}
