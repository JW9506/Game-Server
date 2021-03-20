#include <iostream>
#include "netbus.h"
#include "proto_man.h"
#include "pf_cmd_map.h"
#include "logger.h"
#include "lua_wrapper.h"

int main(int argc, char** argv) {
    lua_wrapper::init();
    init_pf_cmd_map();

    lua_wrapper::execute_file("./scripts/main.lua");

    netbus::instance()->run();
    lua_wrapper::exit();
    return 0;
}
