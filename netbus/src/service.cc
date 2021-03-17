#include "service.h"
#include "proto_man.h"

bool service::on_session_recv_cmd(session* s, struct cmd_msg* msg) {
    return false;
}

void service::on_session_disconnect(session* s) { }
