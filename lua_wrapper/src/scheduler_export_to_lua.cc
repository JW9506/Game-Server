#include "scheduler_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "lua_wrapper.h"
#include "time_list.h"
#include <cstdlib>
#include "small_alloc.h"

#define _new(type, var)                                                        \
    type* var = (type*)small_alloc(sizeof(type));                              \
    memset(var, 0, sizeof(type))
#define _free(mem) small_free(mem)

struct timer_repeat {
    unsigned int handle;
    int repeat_count;
};

static void __on_lua_timer(void* udata) {
    struct timer_repeat* tr = (struct timer_repeat*)udata;
    lua_wrapper::execute_script_handle(tr->handle, 0);
    if (tr->repeat_count == -1) { return; }
    --tr->repeat_count;
    if (tr->repeat_count <= 0) {
        lua_wrapper::remove_script_handle(tr->handle);
        _free(tr);
    }
}

static int lua_scheduler_repeat(lua_State* tolua_S) {
    int handle = toluafix_ref_function(tolua_S, 1, 0);
    if (handle == 0) { goto lua_failed; }

    int after_msec = lua_tointeger(tolua_S, 2);
    if (after_msec <= 0) { goto lua_failed; }

    int repeat_count = lua_tointeger(tolua_S, 3);
    if (repeat_count == 0) { goto lua_failed; }
    if (repeat_count < 0) { repeat_count = -1; }

    int repeat_msec = lua_tointeger(tolua_S, 4);
    if (repeat_msec <= 0) { repeat_msec = after_msec; }

    _new(struct timer_repeat, tr);
    tr->handle = handle;
    tr->repeat_count = repeat_count;
    struct timer* t = schedule_repeat(__on_lua_timer, (void*)tr, after_msec,
                                      repeat_count, repeat_msec);
    t->udata = (void*)tr;
    tolua_pushuserdata(tolua_S, (void*)t);
    return 1;
lua_failed:
    if (handle) { lua_wrapper::remove_script_handle(handle); }
    lua_pushnil(tolua_S);
    return 1;
}

static int lua_scheduler_once(lua_State* tolua_S) {
    int handle = toluafix_ref_function(tolua_S, 1, 0);
    if (handle == 0) { goto lua_failed; }

    int after_msec = lua_tointeger(tolua_S, 2);
    if (after_msec <= 0) { goto lua_failed; }

    _new(struct timer_repeat, tr);
    tr->handle = handle;
    tr->repeat_count = 1;
    struct timer* t = schedule_once(__on_lua_timer, (void*)tr, after_msec);
    t->udata = (void*)tr;
    tolua_pushuserdata(tolua_S, (void*)t);
    return 1;
lua_failed:
    if (handle) { lua_wrapper::remove_script_handle(handle); }
    lua_pushnil(tolua_S);
    return 1;
}

// canceling an ended timer will result in error
static int lua_scheduler_cancel(lua_State* tolua_S) {
    if (!lua_isuserdata(tolua_S, 1)) { goto lua_failed; }
    struct timer* t = (struct timer*)lua_touserdata(tolua_S, 1);
    struct timer_repeat* tr = (struct timer_repeat*)get_timer_udata(t);
    lua_wrapper::remove_script_handle(tr->handle);
    _free(tr);
    cancel_timer(t);
lua_failed:
    return 0;
}

int register_scheduler_export(lua_State* tolua_S) {
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "scheduler", 0);
        tolua_beginmodule(tolua_S, "scheduler");
        tolua_function(tolua_S, "schedule", lua_scheduler_repeat);
        tolua_function(tolua_S, "once", lua_scheduler_once);
        tolua_function(tolua_S, "cancel", lua_scheduler_cancel);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
