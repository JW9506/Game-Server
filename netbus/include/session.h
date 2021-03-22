#pragma once

struct cmd_msg;

class session {
  public:
    unsigned int as_client;
    unsigned int utag;
    session() {
        this->as_client = 0;
        this->utag = 0;
    }

  public:
    virtual void close() = 0;
    virtual void send_data(char* body, int len) = 0;
    virtual const char* get_address(int* port) = 0;
    virtual void send_msg(struct cmd_msg* msg) = 0;
};
