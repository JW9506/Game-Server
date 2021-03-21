#include "service_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "service.h"
#include "session.h"
#include "proto_man.h"
#include "lua_wrapper.h"
#include "logger.h"
#include "service_man.h"
#include <google/protobuf/message.h>

#define SERVICE_FUNCTION_MAPPING "service_function_mapping"

static unsigned int s_function_ref_id;
static unsigned int save_service_function(lua_State* L, int lo, int def) {
    if (!lua_isfunction(L, lo)) return 0;
    s_function_ref_id++;
    lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, s_function_ref_id);
    lua_pushvalue(L, lo);
    lua_rawset(L, -3);
    lua_pop(L, 1);
    return s_function_ref_id;
}
static void get_service_function(lua_State* L, int refid) {
    lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
    lua_rawget(L, LUA_REGISTRYINDEX);
    lua_pushinteger(L, refid);
    lua_rawget(L, -2);
    lua_remove(L, -2);
}
static bool push_service_function(int nHandler) {
    get_service_function(lua_wrapper::lua_state(), nHandler);
    if (!lua_isfunction(lua_wrapper::lua_state(), -1)) {
        log_error(
            "[LUA ERROR] function refid '%d' does not reference a Lua function",
            nHandler);
        lua_pop(lua_wrapper::lua_state(), 1);
        return false;
    }
    return true;
}
static int exe_function(int numArgs) {
    int functionIndex = -(numArgs + 1);
    if (!lua_isfunction(lua_wrapper::lua_state(), functionIndex)) {
        log_error("value at stack [%d] is not function", functionIndex);
        lua_pop(lua_wrapper::lua_state(), numArgs + 1);
        return 0;
    }
    int traceback = 0;
    lua_getglobal(lua_wrapper::lua_state(), "__G__TRACKBACK__");
    if (!lua_isfunction(lua_wrapper::lua_state(), -1)) {
        lua_pop(lua_wrapper::lua_state(), 1);
    } else {
        lua_insert(lua_wrapper::lua_state(), functionIndex - 1);
        traceback = functionIndex - 1;
    }
    int error = 0;
    error = lua_pcall(lua_wrapper::lua_state(), numArgs, 1, traceback);
    if (error) {
        if (traceback == 0) {
            log_error("[LUA ERROR] %s",
                      lua_tostring(lua_wrapper::lua_state(), -1));
            lua_pop(lua_wrapper::lua_state(), 1);
        } else {
            lua_pop(lua_wrapper::lua_state(), 2);
        }
        return 0;
    }
    int ret = 0;
    if (lua_isnumber(lua_wrapper::lua_state(), -1)) {
        ret = (int)lua_tointeger(lua_wrapper::lua_state(), -1);
    } else if (lua_isboolean(lua_wrapper::lua_state(), -1)) {
        ret = (int)lua_toboolean(lua_wrapper::lua_state(), -1);
    }
    lua_pop(lua_wrapper::lua_state(), 1);
    if (traceback) { lua_pop(lua_wrapper::lua_state(), 1); }
    return ret;
}
static int execute_service_function(int nHandler, int numArgs) {
    int ret = 0;
    if (push_service_function(nHandler)) {
        if (numArgs > 0) {
            lua_insert(lua_wrapper::lua_state(), -(numArgs + 1));
        }
        ret = exe_function(numArgs);
    }
    lua_settop(lua_wrapper::lua_state(), 0);
    return ret;
}

static void init_service_function_map(lua_State* L) {
    lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
    lua_newtable(L);
    lua_rawset(L, LUA_REGISTRYINDEX);
}

static void push_proto_message_tolua(const google::protobuf::Message* msg) {
    using namespace google::protobuf;
    using CppType = FieldDescriptor::CppType;
    auto m_pState{ lua_wrapper::lua_state() };
    auto desc{ msg->GetDescriptor() };
    auto refl{ msg->GetReflection() };
    lua_newtable(m_pState);
    for (int i = 0; i < desc->field_count(); ++i) {
        auto fd{ desc->field(i) };
        auto& name{ fd->lowercase_name() };
        lua_pushstring(m_pState, name.c_str());
        if (fd->is_repeated()) {
            lua_newtable(m_pState);
            auto size{ refl->FieldSize(*msg, fd) };
            for (int i = 0; i < size; ++i) {
                char str[32]{ 0 };
                switch (fd->cpp_type()) {
                case CppType::CPPTYPE_DOUBLE:
                    lua_pushnumber(m_pState,
                                   refl->GetRepeatedDouble(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_FLOAT:
                    lua_pushnumber(m_pState,
                                   refl->GetRepeatedFloat(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_INT64:
                    snprintf(str, sizeof(str), "%lld",
                             refl->GetRepeatedInt64(*msg, fd, i));
                    lua_pushstring(m_pState, str);
                    break;
                case CppType::CPPTYPE_UINT64:
                    snprintf(str, sizeof(str), "%llu",
                             refl->GetRepeatedUInt64(*msg, fd, i));
                    lua_pushstring(m_pState, str);
                    break;
                case CppType::CPPTYPE_ENUM:
                    lua_pushinteger(
                        m_pState,
                        (lua_Integer)refl->GetRepeatedEnum(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_INT32:
                    lua_pushinteger(m_pState,
                                    refl->GetRepeatedInt32(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_UINT32:
                    lua_pushinteger(m_pState,
                                    refl->GetRepeatedUInt32(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_STRING: {
                    auto& str{ refl->GetRepeatedString(*msg, fd, i) };
                    lua_pushlstring(m_pState, str.c_str(), str.size());
                    break;
                }
                case CppType::CPPTYPE_BOOL:
                    lua_pushboolean(m_pState,
                                    refl->GetRepeatedBool(*msg, fd, i));
                    break;
                case CppType::CPPTYPE_MESSAGE:
                    push_proto_message_tolua(
                        &refl->GetRepeatedMessage(*msg, fd, i));
                    break;
                default: break;
                }
                lua_rawseti(m_pState, -2, i + 1);
            }
            continue;
        } else {
            char str[32]{ 0 };
            switch (fd->cpp_type()) {
            case CppType::CPPTYPE_DOUBLE:
                lua_pushnumber(m_pState, refl->GetDouble(*msg, fd));
                break;
            case CppType::CPPTYPE_FLOAT:
                lua_pushnumber(m_pState, refl->GetFloat(*msg, fd));
                break;
            case CppType::CPPTYPE_INT64:
                snprintf(str, sizeof(str), "%lld", refl->GetInt64(*msg, fd));
                lua_pushstring(m_pState, str);
                break;
            case CppType::CPPTYPE_UINT64:
                snprintf(str, sizeof(str), "%llu", refl->GetUInt64(*msg, fd));
                lua_pushstring(m_pState, str);
                break;
            case CppType::CPPTYPE_ENUM:
                lua_pushinteger(m_pState, (lua_Integer)refl->GetEnum(*msg, fd));
                break;
            case CppType::CPPTYPE_INT32:
                lua_pushinteger(m_pState, refl->GetInt32(*msg, fd));
                break;
            case CppType::CPPTYPE_UINT32:
                lua_pushinteger(m_pState, refl->GetUInt32(*msg, fd));
                break;
            case CppType::CPPTYPE_STRING: {
                auto& str{ refl->GetString(*msg, fd) };
                lua_pushlstring(m_pState, str.c_str(), str.size());
                break;
            }
            case CppType::CPPTYPE_BOOL:
                lua_pushboolean(m_pState, refl->GetBool(*msg, fd));
                break;
            case CppType::CPPTYPE_MESSAGE:
                push_proto_message_tolua(&refl->GetMessage(*msg, fd));
                break;
            default: break;
            }
        }
        lua_rawset(m_pState, -3);
    }
}

class lua_service : service {
  public:
    unsigned int lua_recv_cmd_action_handle;
    unsigned int lua_disconnect_handle;

  public:
    bool on_session_recv_cmd(session* s, struct cmd_msg* msg) {
        tolua_pushuserdata(lua_wrapper::lua_state(), (void*)s);
        lua_newtable(lua_wrapper::lua_state());
        int index = 1;
        lua_pushinteger(lua_wrapper::lua_state(), msg->stype);
        lua_rawseti(lua_wrapper::lua_state(), -2, index++);
        lua_pushinteger(lua_wrapper::lua_state(), msg->ctype);
        lua_rawseti(lua_wrapper::lua_state(), -2, index++);
        lua_pushinteger(lua_wrapper::lua_state(), msg->utag);
        lua_rawseti(lua_wrapper::lua_state(), -2, index++);
        if (!msg->body) {
            lua_pushnil(lua_wrapper::lua_state());
            lua_rawseti(lua_wrapper::lua_state(), -2, index++);
        } else {
            if (proto_man::proto_type() == PROTO_JSON) {
                lua_pushstring(lua_wrapper::lua_state(), (char*)msg->body);
            } else if (proto_man::proto_type() == PROTO_BUF) {
                push_proto_message_tolua((google::protobuf::Message*)msg->body);
            } else {
                throw "Unsupported proto type";
            }
            lua_rawseti(lua_wrapper::lua_state(), -2, index++);
        }
        execute_service_function(this->lua_recv_cmd_action_handle, 2);
        return true;
    }
    void on_session_disconnect(session* s) {
        tolua_pushuserdata(lua_wrapper::lua_state(), (void*)s);
        execute_service_function(this->lua_disconnect_handle, 1);
    }
};

static int lua_service_register(lua_State* tolua_S) {
    int stype = tolua_tonumber(tolua_S, 1, 0);
    bool ret = false;
    if (!lua_istable(tolua_S, 2)) { goto lua_failed; }
    unsigned int lua_recv_cmd_action_handle;
    unsigned int lua_disconnect_handle;
    lua_getfield(tolua_S, 2, "on_session_recv_cmd");
    lua_getfield(tolua_S, 2, "on_session_disconnect");
    lua_recv_cmd_action_handle = save_service_function(tolua_S, 3, 0);
    lua_disconnect_handle = save_service_function(tolua_S, 4, 0);
    if (lua_recv_cmd_action_handle == 0 || lua_disconnect_handle == 0) {
        goto lua_failed;
    }
    lua_service* s = new lua_service;
    s->lua_recv_cmd_action_handle = lua_recv_cmd_action_handle;
    s->lua_disconnect_handle = lua_disconnect_handle;
    ret = service_man::register_service(stype, (service*)s);
lua_failed:
    lua_pushboolean(tolua_S, (int)ret);
    return 1;
}

int register_service_export(lua_State* tolua_S) {
    init_service_function_map(tolua_S);
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "service", 0);
        tolua_beginmodule(tolua_S, "service");
        tolua_function(tolua_S, "register", lua_service_register);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
