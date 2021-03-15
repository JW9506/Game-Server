#include "uv_session.h"

#ifdef __cpluslus
extern "C" {
#endif
static void write_cb(uv_write_t* req, int status) {
    if (status == 0) { printf("write success\n"); }
}
static void close_cb(uv_handle_t* handle) {
    printf("a client disconnected\n");
    uv_session* s = (uv_session*)handle->data;
    uv_session::destroy(s);
}
static void shutdown_cb(uv_shutdown_t* req, int status) {
    uv_close((uv_handle_t*)req->handle, close_cb);
}
#ifdef __cpluslus
}
#endif

uv_session* uv_session::create() {
    // todo:
    uv_session* uv_s = new uv_session;
    uv_s->init();
    return uv_s;
}

void uv_session::destroy(uv_session* s) {
    // todo:
    s->exit();
    delete s;
}

void uv_session::init() {
    memset(this->c_address, 0, sizeof(this->c_address));
    memset(this->recv_buf, 0, sizeof(this->recv_buf));
    memset(&this->tcp_handle, 0, sizeof(this->tcp_handle));
    this->c_port = 0;
    this->recved = 0;
}

void uv_session::exit() { }

void uv_session::close() {
    auto req{ &this->shutdown };
    memset(req, 0, sizeof(this->shutdown));
    uv_shutdown(req, (uv_stream_t*)&this->tcp_handle, shutdown_cb);
    return;
}

void uv_session::send_data(char* body, size_t len) {
    auto w_req{ &this->w_req };
    auto w_buf{ &this->w_buf };
    *w_buf = uv_buf_init(body, (uint32_t)len);
    uv_write(w_req, (uv_stream_t*)&this->tcp_handle, w_buf, 1, write_cb);
}

const char* uv_session::get_address(int* port) {
    if (port) { *port = this->c_port; }
    return this->c_address;
}
