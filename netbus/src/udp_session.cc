#include "udp_session.h"
#include "proto_man.h"

void udp_session::close() { }

static void udp_send_cb(uv_udp_send_t* req, int status) {
    if (status == 0) { printf("udp send success\n"); }
    free(req);
}

void udp_session::send_data(char* body, int len) {
    uv_buf_t w_buf = uv_buf_init(body, len);
    uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
    uv_udp_send(req, &this->udp_handle, &w_buf, 1, this->addr, udp_send_cb);
}

const char* udp_session::get_address(int* port) {
    if (port) { *port = this->c_port; }
    return this->c_address;
}

void udp_session::send_msg(struct cmd_msg* msg) {
    int out_len;
    unsigned char* encoded_pkg;
    encoded_pkg = proto_man::encode_msg_to_raw(msg, &out_len);
    if (encoded_pkg) {
        this->send_data((char*)encoded_pkg, out_len);
        proto_man::msg_raw_free(encoded_pkg);
    }
}
