#pragma once
#include <string>
#include <map>
#include <google/protobuf/message.h>

#define CMD_HEADER      8

enum {
    PROTO_JSON = 0,
    PROTO_BUF = 1,
};

struct cmd_msg {
    int stype;
    int ctype;
    unsigned int utag;
    void* body;
};

class proto_man {
  public:
    static void init(int proto_type);
    static void register_protobuf_cmd_map(std::map<int, std::string>& map);
    static int proto_type();
    static const char* protobuf_cmd_name(int ctype);
    static bool decode_cmd_msg(unsigned char* cmd, int cmd_len,
                               struct cmd_msg** out_msg);
    static void cmd_msg_free(struct cmd_msg* msg);
    static unsigned char* encode_msg_to_raw(const struct cmd_msg* msg,
                                            int* out_len);
    static void msg_raw_free(unsigned char* raw);
    static google::protobuf::Message* create_message(const char* typeName);
    static void release_message(google::protobuf::Message* m);
};
