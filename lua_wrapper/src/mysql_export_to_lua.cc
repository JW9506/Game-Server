#include "mysql_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "logger.h"
#include "mysql_wrapper.h"
#include "lua_wrapper.h"
#include "mysql.h"

static void open_cb(const char* err, void* context, void* udata) {
    if (err) {
        lua_pushstring(lua_wrapper::lua_state(), err);
        lua_pushnil(lua_wrapper::lua_state());
    } else {
        lua_pushnil(lua_wrapper::lua_state());
        tolua_pushuserdata(lua_wrapper::lua_state(), context);
    }
    lua_wrapper::execute_script_handle((int)udata, 2);
    lua_wrapper::remove_script_handle((int)udata);
}

static int lua_mysql_connect(lua_State* tolua_S) {
    const char* ip = tolua_tostring(tolua_S, 1, NULL);
    if (!ip) { goto lua_failed; }
    int port = (int)tolua_tonumber(tolua_S, 2, NULL);
    const char* db_name = tolua_tostring(tolua_S, 3, NULL);
    if (!db_name) { goto lua_failed; }
    const char* uname = tolua_tostring(tolua_S, 4, NULL);
    if (!uname) { goto lua_failed; }
    const char* pwd = tolua_tostring(tolua_S, 5, NULL);
    if (!pwd) { goto lua_failed; }
    int handle = toluafix_ref_function(tolua_S, 6, NULL);
    mysql_wrapper::connect(ip, port, db_name, uname, pwd, open_cb,
                           (void*)handle);
lua_failed:
    return 0;
}

static int lua_mysql_close(lua_State* tolua_S) {
    void* context = tolua_touserdata(tolua_S, 1, NULL);
    if (!context) { goto lua_failed; }
    mysql_wrapper::close(context);
lua_failed:
    return 0;
}

static void push_mysql_row(MYSQL_ROW row, int num) {
    lua_newtable(lua_wrapper::lua_state());
    int index = 1;
    for (int i = 0; i < num; ++i) {
        if (row[i]) {
            lua_pushstring(lua_wrapper::lua_state(), row[i]);
        } else {
            lua_pushnil(lua_wrapper::lua_state());
        }
        lua_rawseti(lua_wrapper::lua_state(), -2, index);
        ++index;
    }
}

static void query_cb(const char* err, MYSQL_RES* result, void* udata) {
    if (err) {
        lua_pushstring(lua_wrapper::lua_state(), err);
        lua_pushnil(lua_wrapper::lua_state());
    } else {
        lua_pushnil(lua_wrapper::lua_state());
        if (result) {
            lua_newtable(lua_wrapper::lua_state());
            int index = 1;
            int num = (int)mysql_num_fields(result);
            MYSQL_ROW row;
            while (row = mysql_fetch_row(result)) {
                push_mysql_row(row, num);
                lua_rawseti(lua_wrapper::lua_state(), -2, index);
                ++index;
            }
        } else {
            lua_pushnil(lua_wrapper::lua_state());
        }
    }
    lua_wrapper::execute_script_handle((int)udata, 2);
    lua_wrapper::remove_script_handle((int)udata);
}

static int lua_mysql_query(lua_State* tolua_S) {
    void* context = tolua_touserdata(tolua_S, 1, NULL);
    if (!context) { goto lua_failed; }
    const char* sql = (char*)tolua_tostring(tolua_S, 2, NULL);
    if (!sql) { goto lua_failed; }
    int handle = toluafix_ref_function(tolua_S, 3, NULL);
    if (!handle) { goto lua_failed; }
    mysql_wrapper::query(context, sql, query_cb, (void*)handle);
lua_failed:
    return 0;
}

int register_mysql_export(lua_State* tolua_S) {
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "mysql_wrapper", 0);
        tolua_beginmodule(tolua_S, "mysql_wrapper");
        tolua_function(tolua_S, "connect", lua_mysql_connect);
        tolua_function(tolua_S, "close", lua_mysql_close);
        tolua_function(tolua_S, "query", lua_mysql_query);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
