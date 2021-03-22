#pragma once

class session;

class netbus {
  public:
    static netbus* instance();

  private:
    netbus() { }
    void init();

  public:
    void tcp_server(int port);
    void udp_server(int port);
    void ws_server(int port);
    void run();
    void tcp_connect(const char* server_ip, int port,
                     void (*on_connected)(int err, session* s, void* udata),
                     void* udata);
};
