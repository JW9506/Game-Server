#pragma once
#include "lua.hpp"
#include <string>

class lua_wrapper {
  public:
    static void init();
    static void exit();
    static bool execute_file(const std::string& lua_file);
    static lua_State* lua_state();

  public:
    static void reg_func2lua(const char* name, lua_CFunction fun);
    static void add_search_path(const std::string& path);

  public:
    static int execute_script_handle(unsigned int nHandler, int numArgs);
    static void remove_script_handle(unsigned int nHandler);
};
