#include <iostream>
#include "netbus.h"

int main(int argc, char** argv) {
    netbus::instance()->start_tcp_server(6080);
    netbus::instance()->start_tcp_server(6081);
    netbus::instance()->start_ws_server(8001);
    netbus::instance()->run();
    return 0;
}
