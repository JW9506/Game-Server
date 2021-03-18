#pragma once

struct redisReply;

class redis_wrapper {
  public:
    static void connect(char* ip, int port,
                        void (*open_cb)(const char* err, void* context,
                                        void* udata),
                        void* udata = nullptr);
    static void close(void* context);
    static void query(void* context, char* cmd,
                      void (*query_cb)(const char* err, redisReply* result,
                                       void* udata),
                      void* udata = nullptr);
};
