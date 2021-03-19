#pragma once

class netbus {
  public:
    static netbus* instance();

  private:
    netbus() { }

  public:
    void init();
    void tcp_server(int port);
    void udp_server(int port);
    void ws_server(int port);
    void run();
};
