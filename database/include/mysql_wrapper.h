#pragma once
#include <vector>
#include <string>

class mysql_wrapper {
  public:
    static void connect(const char* ip, int port, const char* db_name,
                        const char* uname, const char* pwd,
                        void (*open_cb)(const char* err, void* context,
                                        void* udata),
                        void* udata = NULL);
    static void close(void* context);
    static void
    query(void* context, char* sql,
          void (*query_cb)(const char* err,
                           std::vector<std::vector<std::string>>& result));
};
