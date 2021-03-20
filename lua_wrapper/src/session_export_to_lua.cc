#include "session_export_to_lua.h"
#include <tolua++.h>
#include <tolua_fix.h>
#include "lua_wrapper.h"
#include "session.h"
#include "proto_man.h"
#include "logger.h"
#include <google/protobuf/message.h>

static int lua_session_close(lua_State* tolua_S) {
    session* s = (session*)tolua_touserdata(tolua_S, 1, 0);
    if (!s) { goto lua_failed; }
    s->close();
lua_failed:
    return 0;
}

static google::protobuf::Message*
lua_table_to_protobuf(lua_State* L, int stack_idx, const char* msg_name) {
    using namespace google::protobuf;
    using CppType = FieldDescriptor::CppType;
    auto msg{ proto_man::create_message(msg_name) };
    if (!msg) {
        log_error("Can't find message %s", msg_name);
        return NULL;
    }
    auto reflec{ msg->GetReflection() };
    auto desc{ msg->GetDescriptor() };
    for (int i = 0; i < desc->field_count(); ++i) {
        auto fd{ desc->field(i) };
        auto& name{ fd->name() };
        auto is_repeated{ fd->is_repeated() };
        auto is_required{ fd->is_required() };
        lua_pushstring(L, name.c_str());
        lua_rawget(L, stack_idx);
        auto is_nil{ lua_isnil(L, -1) };
        if (is_repeated) {
            if (is_nil) {
                lua_pop(L, 1);
                continue;
            } else {
                if (!lua_istable(L, -1)) {
                    log_error("can't find required repeated field %s",
                              name.c_str());
                    proto_man::release_message(msg);
                    return NULL;
                }
            }
            lua_pushnil(L);
            for (; lua_next(L, -2) != 0;) {
                switch (fd->cpp_type()) {
                case CppType::CPPTYPE_DOUBLE:
                    reflec->AddDouble(msg, fd, (double)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_FLOAT:
                    reflec->AddFloat(msg, fd, (float)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_INT64:
                    reflec->AddInt64(msg, fd, (int64_t)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_UINT64:
                    reflec->AddUInt64(msg, fd,
                                      (uint64_t)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_ENUM: {
                    auto enumDescriptor{ fd->enum_type() };
                    auto valueDescriptor{ enumDescriptor->FindValueByNumber(
                        luaL_checknumber(L, -1)) };
                    reflec->AddEnum(msg, fd, valueDescriptor);
                    break;
                }
                case CppType::CPPTYPE_INT32:
                    reflec->AddInt32(msg, fd, (int32_t)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_UINT32:
                    reflec->AddUInt32(msg, fd,
                                      (uint32_t)luaL_checknumber(L, -1));
                    break;
                case CppType::CPPTYPE_STRING: {
                    size_t size = 0;
                    const char* value = luaL_checklstring(L, -1, &size);
                    reflec->AddString(msg, fd, std::string{ value, size });
                    break;
                }
                case CppType::CPPTYPE_BOOL:
                    reflec->AddBool(msg, fd, lua_toboolean(L, -1));
                    break;
                case CppType::CPPTYPE_MESSAGE: {
                    auto msg_name{ fd->message_type()->name().c_str() };
                    auto value{ lua_table_to_protobuf(L, lua_gettop(L), msg_name) };
                    if (!value) {
                        log_error("convert to message %s failed", msg_name);
                        proto_man::release_message(value);
                        return NULL;
                    }
                    auto _msg{ reflec->AddMessage(msg, fd) };
                    _msg->CopyFrom(*value);
                    proto_man::release_message(value);
                    break;
                }
                default: break;
                }
                lua_pop(L, 1);
            }
        } else {
            if (is_required) {
                if (is_nil) {
                    log_error("can't find required field %s", name);
                    proto_man::release_message(msg);
                    return NULL;
                }
            } else {
                if (is_nil) {
                    lua_pop(L, 1);
                    continue;
                }
            }
            switch (fd->cpp_type()) {
            case CppType::CPPTYPE_DOUBLE:
                reflec->SetDouble(msg, fd, (double)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_FLOAT:
                reflec->SetFloat(msg, fd, (float)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_INT64:
                reflec->SetInt64(msg, fd, (int64_t)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_UINT64:
                reflec->SetUInt64(msg, fd, (uint64_t)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_ENUM: {
                auto enumDescriptor{ fd->enum_type() };
                auto valueDescriptor{ enumDescriptor->FindValueByNumber(
                    luaL_checknumber(L, -1)) };
                reflec->SetEnum(msg, fd, valueDescriptor);
                break;
            }
            case CppType::CPPTYPE_INT32:
                reflec->SetInt32(msg, fd, (int32_t)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_UINT32:
                reflec->SetUInt32(msg, fd, (uint32_t)luaL_checknumber(L, -1));
                break;
            case CppType::CPPTYPE_STRING: {
                size_t size = 0;
                const char* value = luaL_checklstring(L, -1, &size);
                reflec->SetString(msg, fd, std::string{ value, size });
                break;
            }
            case CppType::CPPTYPE_BOOL:
                reflec->SetBool(msg, fd, lua_toboolean(L, -1));
                break;
            case CppType::CPPTYPE_MESSAGE: {
                auto msg_name{ fd->message_type()->name().c_str() };
                auto value{ lua_table_to_protobuf(L, lua_gettop(L), msg_name) };
                if (!value) {
                    log_error("convert to message %s failed", msg_name);
                    proto_man::release_message(value);
                    return NULL;
                }
                auto _msg{ reflec->MutableMessage(msg, fd) };
                _msg->CopyFrom(*value);
                proto_man::release_message(value);
                break;
            }
            default: break;
            }
        }
        lua_pop(L, 1);
    }
    return msg;
}

static int lua_send_msg(lua_State* tolua_S) {
    session* s = (session*)tolua_touserdata(tolua_S, 1, 0);
    if (!s) { goto lua_failed; }
    if (!lua_istable(tolua_S, 2)) { goto lua_failed; }

    struct cmd_msg msg;
    int n = luaL_len(tolua_S, 2);
    if (n != 4) { goto lua_failed; }

    lua_pushnumber(tolua_S, 1);
    lua_gettable(tolua_S, -2);
    msg.stype = lua_tointeger(tolua_S, -1);
    lua_pop(tolua_S, 1);

    lua_pushnumber(tolua_S, 2);
    lua_gettable(tolua_S, -2);
    msg.ctype = lua_tointeger(tolua_S, -1);
    lua_pop(tolua_S, 1);

    lua_pushnumber(tolua_S, 3);
    lua_gettable(tolua_S, -2);
    msg.utag = lua_tointeger(tolua_S, -1);
    lua_pop(tolua_S, 1);

    lua_pushnumber(tolua_S, 4);
    lua_gettable(tolua_S, -2);
    if (proto_man::proto_type() == PROTO_JSON) {
        msg.body = (void*)lua_tostring(tolua_S, -1);
        s->send_msg(&msg);
    } else if (proto_man::proto_type() == PROTO_BUF) {
        if (!lua_istable(tolua_S, -1)) {
            msg.body = NULL;
            s->send_msg(&msg);
        } else {
            const char* msg_name = proto_man::protobuf_cmd_name(msg.ctype);
            msg.body = (void*)lua_table_to_protobuf(
                tolua_S, lua_gettop(tolua_S), msg_name);
            s->send_msg(&msg);
            proto_man::release_message((google::protobuf::Message*)msg.body);
        }
    } else {
        throw "not supported";
    }
    lua_settop(tolua_S, 0);
lua_failed:
    return 0;
}

static int lua_get_address(lua_State* tolua_S) {
    session* s = (session*)tolua_touserdata(tolua_S, 1, 0);
    if (!s) { goto lua_failed; }
    int client_port;
    const char* ip = s->get_address(&client_port);
    lua_pushstring(tolua_S, ip);
    lua_pushinteger(tolua_S, client_port);
    return 2;
lua_failed:
    return 0;
}

int register_session_export(lua_State* tolua_S) {
    lua_getglobal(tolua_S, "_G");
    if (lua_istable(tolua_S, -1)) {
        tolua_open(tolua_S);
        tolua_module(tolua_S, "session", 0);
        tolua_beginmodule(tolua_S, "session");
        tolua_function(tolua_S, "close", lua_session_close);
        tolua_function(tolua_S, "send_msg", lua_send_msg);
        tolua_function(tolua_S, "get_address", lua_get_address);
        tolua_endmodule(tolua_S);
    }
    lua_pop(tolua_S, 1);
    return 0;
}
