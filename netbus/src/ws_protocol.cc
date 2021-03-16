#include "ws_protocol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <http_parser.h>
#include "sha1.h"
#include "session.h"
#include "base64_encoder.h"

#define SHA1_DIGEST_SIZE 64

#ifdef __cplusplus
extern "C" {
#endif
static char* wb_magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
static char* wb_accept = "HTTP/1.1 101 Switching Protocols\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Accept: %s\r\n"
                         "WebSocket-Protocol: chat\r\n\r\n";
static char field_sec_key[256];
static char value_sec_key[256];
static int is_sec_key = 0;
static int has_sec_key = 0;
static int is_shake_ended = 0;

static int p_ws_header_field(http_parser* p, const char* at, size_t length) {
    if (strncmp(at, "Sec-WebSocket-Key", length) == 0) {
        is_sec_key = 1;
    } else {
        is_sec_key = 0;
    }
    return 0;
}
static int p_ws_header_value(http_parser* p, const char* at, size_t length) {
    if (!is_sec_key) { return 0; }
    strncpy_s(value_sec_key, sizeof(value_sec_key), at, length);
    value_sec_key[length] = 0;
    has_sec_key = 1;
    return 0;
}

static int p_ws_message_end(http_parser* p) {
    is_shake_ended = 1;
    return 0;
}

#ifdef __cplusplus
}
#endif

bool ws_protocol::ws_shake_hand(session* s, char* body, int len) {
    http_parser_settings settings;
    http_parser_settings_init(&settings);
    settings.on_header_field = p_ws_header_field;
    settings.on_header_value = p_ws_header_value;
    settings.on_message_complete = p_ws_message_end;
    http_parser p;
    http_parser_init(&p, HTTP_REQUEST);
    is_sec_key = 0;
    has_sec_key = 0;
    is_shake_ended = 0;
    http_parser_execute(&p, &settings, body, len);
    // todo: how is this possible? (async), data arrive in discrete time frame,
    // which means cb is called multiple times
    if (has_sec_key && is_shake_ended) {
#ifdef _DEBUG
        printf("Sec-WebSocket-Key: %s .\n", value_sec_key);
#endif
        static char key_magic[512];
        static char sha1_key_magic[SHA1_DIGEST_SIZE];
        static char send_client[512];
        static char base64_buf[512];
        sprintf_s(key_magic, sizeof(key_magic), "%s%s", value_sec_key,
                  wb_magic);
        sha1(key_magic, sizeof(key_magic), sha1_key_magic,
             sizeof(sha1_key_magic));
        size_t base64_buf_len;
        base64_encode(sha1_key_magic, base64_buf, &base64_buf_len);
        sprintf_s(send_client, sizeof(send_client), wb_accept, base64_buf);
#ifdef _DEBUG
        printf("Before sha1 str: %s\nBase64 encoded: %s\n", key_magic,
               base64_buf);
#endif
        s->send_data(send_client, (int)strlen(send_client));
        return true;
    }
    return false;
}

bool ws_protocol::read_ws_header(unsigned char* recv_data, int recv_len,
                                 int* pkg_size, int* out_header_size) {
    if (recv_data[0] != 0x81 && recv_data[0] != 0x82) { return false; }
    if (recv_len < 2) { return false; }
    unsigned int data_len = recv_data[1] & 0x7f;
    int head_size = 2;
    if (data_len == 126) {
        head_size += 2;
        if (recv_len < head_size) { return false; }
        data_len = recv_data[3] | (recv_data[2] << 8);
    } else if (data_len == 127) {
        head_size += 8;
        if (recv_len < head_size) { return false; }
        unsigned int low = recv_data[5] | (recv_data[4] << 8) |
                           (recv_data[3] << 16) | (recv_data[2] << 24);
        unsigned int high = recv_data[9] | (recv_data[8] << 8) |
                            (recv_data[9] << 16) | (recv_data[6] << 24);
        data_len = low;
    }
    // todo: check
    head_size += 4;
    *pkg_size = data_len + head_size;
    *out_header_size = head_size;
    return true;
}

void ws_protocol::parse_ws_recv_data(unsigned char* raw_data,
                                     unsigned char* mask, int raw_len) {
    for (int i = 0; i < raw_len; ++i) {
        raw_data[i] = raw_data[i] ^ mask[i % 4];
    }
}

unsigned char* ws_protocol::package_ws_send_data(const unsigned char* raw_data,
                                                 int len, int* ws_data_len) {
    int head_size = 2;
    if (len > 125 && len < 65536) {
        head_size += 2;
    } else if (len >= 65536) {
        head_size += 8;
        return NULL;
    }
    // cache malloc
    unsigned char* data_buf = (unsigned char*)malloc(head_size + len);
    data_buf[0] = 0x81;
    if (len <= 125) {
        data_buf[1] = (char)len;
    } else if (len > 125 && len < 65536) {
        data_buf[1] = 126;
        data_buf[2] = (len & 0x0000ff00) >> 8;
        data_buf[3] = len & 0x000000ff;
    }
    memcpy(data_buf + head_size, raw_data, len);
    *ws_data_len = head_size + len;
    return data_buf;
}

void ws_protocol::free_ws_send_pkg_data(unsigned char* ws_pkg) {
    // cache free
    free(ws_pkg);
}
