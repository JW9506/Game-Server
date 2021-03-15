#pragma once
#include <uv.h>
#include "session.h"

#define RECV_LEN 4096

enum { TCP_SOCKET, WS_SOCKET };

class uv_session : session {
  public:
    uv_tcp_t tcp_handle;
    char c_address[32];
    int c_port;

    uv_shutdown_t shutdown;
    uv_write_t w_req;
    uv_buf_t w_buf;

  private:
    void init();
    void exit();

  public:
    char recv_buf[RECV_LEN];
    int recved;
    int socket_type;

  public:
    static uv_session* create();
    static void destroy(uv_session* s);

  public:
    virtual void close();
    virtual void send_data(char* body, size_t len);
    virtual const char* get_address(int* port);
};
