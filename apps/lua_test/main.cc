#include <iostream>
#include "netbus.h"
#include "proto_man.h"
#include "pf_cmd_map.h"
#include "logger.h"
#include "lua_wrapper.h"

int main(int argc, char** argv) {
    lua_wrapper::init();
    init_pf_cmd_map();

    if (argc != 3) {
        lua_wrapper::add_search_path("./scripts/");
        lua_wrapper::execute_file("./scripts/main.lua");
    } else {
        std::string path{ argv[1] };
        if (path.back() != '/') { path += '/'; }
        lua_wrapper::add_search_path(path);
        lua_wrapper::execute_file(path + argv[2]);
    }

    netbus::instance()->run();
    lua_wrapper::exit();
    return 0;
}
