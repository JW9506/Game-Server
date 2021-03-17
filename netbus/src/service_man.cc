#include "service_man.h"
#include "service.h"
#include "proto_man.h"
#include <cstring>

#define MAX_SERVICE 1024
static service* g_service_set[MAX_SERVICE];

bool service_man::register_service(int stype, service* s) {
    if (stype < 0 || stype >= MAX_SERVICE) { return false; }
    if (g_service_set[stype]) { return false; }
    g_service_set[stype] = s;
    return true;
}

bool service_man::on_recv_cmd_msg(session* s, struct cmd_msg* msg) {
    if (!g_service_set[msg->stype]) { return false; }
    return g_service_set[msg->stype]->on_session_recv_cmd(s, msg);
}

void service_man::on_session_disconnect(session* s) {
    for (int i = 0; i < MAX_SERVICE; ++i) {
        if (!g_service_set[i]) { continue; }
        g_service_set[i]->on_session_disconnect(s);
    }
}

void service_man::init() { memset(g_service_set, 0, sizeof(g_service_set)); }
