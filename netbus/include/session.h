#pragma once

class session {
  public:
    virtual void close() = 0;
    virtual void send_data(char* body, size_t len) = 0;
    virtual const char* get_address(int* port) = 0;
};
