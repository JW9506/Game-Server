#pragma once
struct st_mysql_res;

class mysql_wrapper {
  public:
    static void connect(const char* ip, int port, const char* db_name,
                        const char* uname, const char* pwd,
                        void (*open_cb)(const char* err, void* context,
                                        void* udata),
                        void* udata = nullptr);
    static void close(void* context);
    static void query(void* context, const char* sql,
                      void (*query_cb)(const char* err,
                                       struct st_mysql_res* result,
                                       void* udata),
                      void* udata = nullptr);
};
