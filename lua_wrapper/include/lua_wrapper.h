#pragma once
#include "lua.hpp"

class lua_wrapper {
  public:
    static void init();
    static void exit();
    static bool exe_lua_file(const char* lua_file);
    static lua_State* lua_state();

  public:
    static void reg_func2lua(const char* name, lua_CFunction fun);

  public:
    static int execute_script_handle(unsigned int nHandler, int numArgs);
    static void remove_script_handle(unsigned int nHandler);
};
