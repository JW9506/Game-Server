#include <iostream>
#include "netbus.h"
#include "proto_man.h"
#include "pf_cmd_map.h"
#include "logger.h"
#include "lua_wrapper.h"

int main(int argc, char** argv) {
    proto_man::init(PROTO_BUF);
    init_pf_cmd_map();
    logger::init("logger/netbus", "netbus_log", 1);

    lua_wrapper::init();
    lua_wrapper::execute_file("./scripts/main.lua");

    netbus::instance()->tcp_server(6080);
    netbus::instance()->tcp_server(6081);
    netbus::instance()->ws_server(8001);
    netbus::instance()->udp_server(8002);

    netbus::instance()->run();
    lua_wrapper::exit();
    return 0;
}
