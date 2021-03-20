#include "netbus_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "netbus.h"

static int lua_netbus_init(lua_State* tolua_S) {
    netbus::instance();
    return 0;
}
static int lua_netbus_tcp(lua_State* tolua_S) {
    int argc = lua_gettop(tolua_S);
    if (argc != 1) { goto lua_failed; }
    int port = lua_tointeger(tolua_S, 1);
    netbus::instance()->tcp_server(port);
lua_failed:
    return 0;
}
static int lua_netbus_udp(lua_State* tolua_S) {
    int argc = lua_gettop(tolua_S);
    if (argc != 1) { goto lua_failed; }
    int port = lua_tointeger(tolua_S, 1);
    netbus::instance()->udp_server(port);
lua_failed:
    return 0;
}
static int lua_netbus_ws(lua_State* tolua_S) {
    int argc = lua_gettop(tolua_S);
    if (argc != 1) { goto lua_failed; }
    int port = lua_tointeger(tolua_S, 1);
    netbus::instance()->ws_server(port);
lua_failed:
    return 0;
}
static int lua_netbus_run(lua_State* tolua_S) {
    netbus::instance()->run();
    return 0;
}
int register_netbus_export(lua_State* tolua_S) {
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "netbus", 0);
        tolua_beginmodule(tolua_S, "netbus");
        tolua_function(tolua_S, "init", lua_netbus_init);
        tolua_function(tolua_S, "tcp_server", lua_netbus_tcp);
        tolua_function(tolua_S, "udp_server", lua_netbus_udp);
        tolua_function(tolua_S, "ws_server", lua_netbus_ws);
        tolua_function(tolua_S, "run", lua_netbus_run);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
