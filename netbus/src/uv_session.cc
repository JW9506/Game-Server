#include "uv_session.h"
#include "ws_protocol.h"
#include "tp_protocol.h"
#include "cache_alloc.h"

#define SESSION_CACHE_CAPACITY 6000
#define WQ_CACHE_CAPACITY      4096
#define WBUF_CACHE_CAPACITY    1024
#define CMD_CACHE_SIZE         1024
static struct cache_allocer* session_allocator = NULL;
static struct cache_allocer* wr_allocator = NULL;
struct cache_allocer* wbuf_allocator = NULL;

void init_session_allocer() {
    if (!session_allocator) {
        session_allocator =
            create_cache_allocer(SESSION_CACHE_CAPACITY, sizeof(uv_session));
    }
    if (!wr_allocator) {
        wr_allocator =
            create_cache_allocer(WQ_CACHE_CAPACITY, sizeof(uv_write_t));
    }
    if (!wbuf_allocator) {
        wbuf_allocator =
            create_cache_allocer(WBUF_CACHE_CAPACITY, CMD_CACHE_SIZE);
    }
}

#ifdef __cplusplus
extern "C" {
#endif
static void write_cb(uv_write_t* req, int status) {
    if (status == 0) { printf("write success\n"); }
    cache_free(wr_allocator, req);
}
static void close_cb(uv_handle_t* handle) {
    printf("a client disconnected\n");
    uv_session* s = (uv_session*)handle->data;
    uv_session::destroy(s);
}
static void shutdown_cb(uv_shutdown_t* req, int status) {
    uv_close((uv_handle_t*)req->handle, close_cb);
}
#ifdef __cplusplus
}
#endif

uv_session* uv_session::create() {
    uv_session* uv_s =
        (uv_session*)cache_alloc(session_allocator, sizeof(uv_session));
    uv_s->uv_session::uv_session();
    uv_s->init();
    return uv_s;
}

void uv_session::destroy(uv_session* s) {
    s->exit();
    s->uv_session::~uv_session();
    cache_free(session_allocator, s);
}

void uv_session::init() {
    memset(this->c_address, 0, sizeof(this->c_address));
    memset(this->recv_buf, 0, sizeof(this->recv_buf));
    memset(&this->tcp_handle, 0, sizeof(this->tcp_handle));
    this->c_port = 0;
    this->recved = 0;
    this->did_shake_hand = 0;
    this->long_pkg = NULL;
    this->long_pkg_size = 0;
}

void uv_session::exit() { }

void uv_session::close() {
    auto req{ &this->shutdown };
    memset(req, 0, sizeof(this->shutdown));
    uv_shutdown(req, (uv_stream_t*)&this->tcp_handle, shutdown_cb);
    return;
}

void uv_session::send_data(char* body, int len) {
    uv_write_t* w_req =
        (uv_write_t*)cache_alloc(wr_allocator, sizeof(uv_write_t));
    uv_buf_t w_buf;
    if (this->socket_type == WS_SOCKET) {
        if (this->did_shake_hand) {
            int ws_pkg_len;
            unsigned char* ws_pkg = ws_protocol::package_ws_send_data(
                (unsigned char*)body, (int)len, &ws_pkg_len);
            w_buf = uv_buf_init((char*)ws_pkg, (uint32_t)ws_pkg_len);
            uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1,
                     write_cb);
            /* async problem? */
            ws_protocol::free_ws_send_pkg_data(ws_pkg);
        } else {
            w_buf = uv_buf_init(body, (uint32_t)len);
            uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1,
                     write_cb);
        }
    } else {
        int tp_pkg_len;
        unsigned char* tp_pkg =
            tp_protocol::package((unsigned char*)body, (int)len, &tp_pkg_len);
        w_buf = uv_buf_init((char*)tp_pkg, (uint32_t)tp_pkg_len);
        uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1, write_cb);
        tp_protocol::release_package(tp_pkg);
    }
}

const char* uv_session::get_address(int* port) {
    if (port) { *port = this->c_port; }
    return this->c_address;
}
