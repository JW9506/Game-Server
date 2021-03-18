#include "redis_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "redis_wrapper.h"
#include "lua_wrapper.h"
#include <hiredis/hiredis.h>

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

static int lua_redis_connect(lua_State* tolua_S) {
    const char* ip = tolua_tostring(tolua_S, 1, NULL);
    if (!ip) { goto lua_failed; }
    int port = (int)tolua_tonumber(tolua_S, 2, NULL);
    int handle = toluafix_ref_function(tolua_S, 3, NULL);
    redis_wrapper::connect(ip, port, open_cb, (void*)handle);
lua_failed:
    return 0;
}

static int lua_redis_close(lua_State* tolua_S) {
    void* context = tolua_touserdata(tolua_S, 1, NULL);
    if (!context) { goto lua_failed; }
    redis_wrapper::close(context);
lua_failed:
    return 0;
}

static void push_result_to_lua(redisReply* result) {
    switch (result->type) {
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_STRING:
        lua_pushstring(lua_wrapper::lua_state(), result->str);
        break;
    case REDIS_REPLY_INTEGER:
        lua_pushinteger(lua_wrapper::lua_state(), result->integer);
        break;
    case REDIS_REPLY_NIL: lua_pushnil(lua_wrapper::lua_state()); break;
    case REDIS_REPLY_ARRAY: {
        lua_newtable(lua_wrapper::lua_state());
        int index = 1;
        for (int i = 0; i < result->elements; ++i) {
            push_result_to_lua(result->element[i]);
            lua_rawseti(lua_wrapper::lua_state(), -2, index);
            ++index;
        }
        break;
    }
    default: break;
    }
}

static void query_cb(const char* err, redisReply* result, void* udata) {
    if (err) {
        lua_pushstring(lua_wrapper::lua_state(), err);
        lua_pushnil(lua_wrapper::lua_state());
    } else {
        lua_pushnil(lua_wrapper::lua_state());
        if (result) {
            push_result_to_lua(result);
        } else {
            lua_pushnil(lua_wrapper::lua_state());
        }
    }
    lua_wrapper::execute_script_handle((int)udata, 2);
    lua_wrapper::remove_script_handle((int)udata);
}

static int lua_redis_query(lua_State* tolua_S) {
    void* context = tolua_touserdata(tolua_S, 1, NULL);
    if (!context) { goto lua_failed; }
    const char* cmd = (char*)tolua_tostring(tolua_S, 2, NULL);
    if (!cmd) { goto lua_failed; }
    int handle = toluafix_ref_function(tolua_S, 3, NULL);
    if (!handle) { goto lua_failed; }
    redis_wrapper::query(context, cmd, query_cb, (void*)handle);
lua_failed:
    return 0;
}

int register_redis_export(lua_State* tolua_S) {
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "redis_wrapper", 0);
        tolua_beginmodule(tolua_S, "redis_wrapper");
        tolua_function(tolua_S, "connect", lua_redis_connect);
        tolua_function(tolua_S, "close", lua_redis_close);
        tolua_function(tolua_S, "query", lua_redis_query);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
