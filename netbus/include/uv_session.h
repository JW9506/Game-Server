#pragma once
#include <uv.h>
#include "session.h"

#define RECV_LEN 4096

enum { TCP_SOCKET, WS_SOCKET };

void init_session_allocer();

class uv_session : session {
  public:
    uv_tcp_t tcp_handle;
    char c_address[32];
    int c_port;

    uv_shutdown_t shutdown;

  public:
    int did_shake_hand;

  private:
    void init();
    void exit();

  public:
    char recv_buf[RECV_LEN];
    int recved;
    int socket_type;
    char* long_pkg;
    int long_pkg_size;

  public:
    static uv_session* create();
    static void destroy(uv_session* s);

  public:
    virtual void close();
    virtual void send_data(char* body, int len);
    virtual const char* get_address(int* port);
};
