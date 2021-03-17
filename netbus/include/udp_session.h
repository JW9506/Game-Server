#pragma once
#include <uv.h>
#include "session.h"

class udp_session : session {
  public:
    uv_udp_t udp_handle;
    char c_address[32];
    int c_port;
    const struct sockaddr* addr;

  public:
    virtual void close();
    virtual void send_data(char* body, int len);
    virtual const char* get_address(int* port);
    virtual void send_msg(struct cmd_msg* msg);
};
