#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "tp_protocol.h"
#include "proto_man.h"
#include "game.pb.h"
#include "pf_cmd_map.h"
#ifdef WIN32
#include <winsock.h>
#include <Windows.h>
#endif

int main(int argc, char** argv) {
    int port;
    if (argc < 2) {
        port = 6080;
        printf("%s [port], default to %d\n", argv[0], port);
    } else {
        port = atoi(argv[1]);
    }
    int ret;
#ifdef WIN32
    WSADATA wsaData;
    ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
        printf("WSAStart up failed\n");
        return -1;
    }
#endif
    init_pf_cmd_map();

    LoginReq req;
    req.set_age(10);
    req.set_name("jayden");
    req.set_email("q@gmail.com");

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { goto failed; }
    struct sockaddr_in sockaddr;
    sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    ret = connect(s, (struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (ret != 0) { goto failed; }
    listen(s, 1);

    int len = req.ByteSize();
    char* data = (char*)malloc(CMD_HEADER + len);
    memset(data, 0, CMD_HEADER + len);
    req.SerializePartialToArray(data + CMD_HEADER, len);
    int pkg_len;
    unsigned char* pkg_data =
        tp_protocol::package((unsigned char*)data, CMD_HEADER + len, &pkg_len);
    send(s, (char*)pkg_data, pkg_len, 0);
    free(data);
    tp_protocol::release_package(pkg_data);

    static char recv_buf[256];
    int recv_len = recv(s, recv_buf, sizeof(recv_buf), 0);
    if (!recv_len) {
        printf("read nothing from server\n");
        goto failed;
    }
    int pkg_size, header_size;
    tp_protocol::read_header((unsigned char*)recv_buf, recv_len, &pkg_size,
                             &header_size);

    req.ParseFromArray(recv_buf + header_size + CMD_HEADER,
                       pkg_size - header_size - CMD_HEADER);
    printf(">> %s: %d, %s", req.name().c_str(), req.age(), req.email().c_str());
failed:
    if (s != INVALID_SOCKET) {
        closesocket(s);
        s = INVALID_SOCKET;
    }

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
