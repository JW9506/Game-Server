#include "proto_man.h"
#include <cstdio>
#include <cstring>
#include <google/protobuf/message.h>

#define MAX_PF_MAP_SIZE 1024

static int g_proto_type = PROTO_BUF;
static char* g_pf_map[MAX_PF_MAP_SIZE];
static int g_cmd_count = 0;

static google::protobuf::Message* create_message(const char* typeName) {
    google::protobuf::Message* message{};
    auto descriptor{ google::protobuf::DescriptorPool::generated_pool()
                         ->FindMessageTypeByName(typeName) };
    if (descriptor) {
        auto prototype{ google::protobuf::MessageFactory::generated_factory()
                            ->GetPrototype(descriptor) };
        if (prototype) { message = prototype->New(); }
    }
    return message;
}

static void release_message(google::protobuf::Message* m) { delete m; }

void proto_man::init(int proto_type) { g_proto_type = proto_type; }

int proto_man::proto_type() { return g_proto_type; }

void proto_man::register_pf_cmd_map(char** pf_map, int len) {
    len = (MAX_PF_MAP_SIZE - g_cmd_count) < len
              ? (MAX_PF_MAP_SIZE - g_cmd_count)
              : len;
    for (int i = 0; i < len; ++i) {
        g_pf_map[g_cmd_count + i] = _strdup(pf_map[i]);
    }
    g_cmd_count += len;
}

// stype(2) | ctype(2) | utag(4) | body
bool proto_man::decode_cmd_msg(unsigned char* cmd, int cmd_len,
                               struct cmd_msg** out_msg) {
    *out_msg = NULL;
    if (cmd_len < CMD_HEADER) { return false; }
    struct cmd_msg* msg = (struct cmd_msg*)malloc(sizeof(struct cmd_msg));
    memset(msg, 0, sizeof(struct cmd_msg));
    // small endian
    msg->stype = cmd[0] | (cmd[1] << 8);
    msg->ctype = cmd[2] | (cmd[3] << 8);
    msg->utag = cmd[4] | (cmd[5] << 8) | (cmd[6] << 16) | (cmd[7] << 24);
    msg->body = NULL;
    *out_msg = msg;
    if (cmd_len == CMD_HEADER) { return true; }

    // decrypt
    // --

    if (g_proto_type == PROTO_JSON) {
        int json_len = cmd_len - CMD_HEADER;
        char* json_str = (char*)malloc(json_len + 1);
        memcpy(json_str, cmd + CMD_HEADER, json_len);
        json_str[json_len] = 0;
        msg->body = (void*)json_str;
    } else {
        if (msg->ctype < 0 || msg->ctype >= MAX_PF_MAP_SIZE ||
            g_pf_map[msg->ctype] == NULL) {
            goto failed;
        }
        google::protobuf::Message* p_m = create_message(g_pf_map[msg->ctype]);
        if (p_m == NULL) { goto failed; }
        if (!p_m->ParseFromArray(cmd + CMD_HEADER, cmd_len - CMD_HEADER)) {
            release_message(p_m);
            goto failed;
        }
        msg->body = p_m;
    }

    return true;
failed:
    free(msg);
    *out_msg = NULL;
    return false;
}

void proto_man::cmd_msg_free(struct cmd_msg* msg) {
    if (msg->body) {
        if (g_proto_type == PROTO_JSON) {
            free(msg->body);
            msg->body = NULL;
        } else {
            google::protobuf::Message* pm =
                (google::protobuf::Message*)msg->body;
            release_message(pm);
            msg->body = NULL;
        }
    }
    free(msg);
}

unsigned char* proto_man::encode_msg_to_raw(const struct cmd_msg* msg,
                                            int* out_len) {
    int raw_len = 0;
    *out_len = 0;
    unsigned char* raw_data = NULL;
    if (g_proto_type == PROTO_JSON) {
        char* json_str = (char*)msg->body;
        int json_str_len = (int)strlen(json_str);
        int len = CMD_HEADER + json_str_len + 1;
        raw_data = (unsigned char*)malloc(len);
        memcpy(raw_data + CMD_HEADER, json_str, json_str_len - CMD_HEADER - 1);
        raw_data[len] = 0;
        *out_len = len;
    } else { // protobuf
        google::protobuf::Message* p_m = (google::protobuf::Message*)msg->body;
        int pf_len = p_m->ByteSize();
        raw_data = (unsigned char*)malloc(CMD_HEADER + pf_len);
        if (!p_m->SerializePartialToArray(raw_data + CMD_HEADER, pf_len)) {
            free(raw_data);
            return NULL;
        }
        *out_len = pf_len + CMD_HEADER;
    }
    raw_data[0] = msg->stype & 0xff;
    raw_data[1] = (msg->stype & 0xff00) >> 8;
    raw_data[2] = msg->ctype & 0xff;
    raw_data[3] = (msg->ctype & 0xff00) >> 8;
    memcpy(raw_data+4, &msg->utag, 4);
    return raw_data;
}

void proto_man::msg_raw_free(unsigned char* raw) {
    free(raw);
}
