#pragma once

class netbus {
  public:
    static netbus* instance();

  private:
    netbus() { }

  public:
    void init();
    void start_tcp_server(int port);
    void start_udp_server(int port);
    void start_ws_server(int port);
    void run();
};
