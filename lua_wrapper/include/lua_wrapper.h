#pragma once
#include "lua.hpp"

class lua_wrapper {
  public:
    static void init();
    static void exit();
    static bool exe_lua_file(const char* lua_file);

  public:
    static void reg_func2lua(const char* name, lua_CFunction fun);
};
