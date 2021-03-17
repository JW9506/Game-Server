#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "tp_protocol.h"
#ifdef WIN32
#include <winsock.h>
#include <Windows.h>
#endif

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("%s [port]\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    int ret;
#ifdef WIN32
    WORD wVersionRequested;
    WSADATA wsaData;
    wVersionRequested = MAKEWORD(2, 2);
    ret = WSAStartup(wVersionRequested, &wsaData);
    if (ret != 0) {
        printf("WSAStart up failed\n");
        system("pause");
        return -1;
    }
#endif

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) { goto failed; }
    struct sockaddr_in sockaddr;
    sockaddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    ret = connect(s, (const struct sockaddr*)&sockaddr, sizeof(sockaddr));
    if (ret != 0) { goto failed; }
    ret = listen(s, 1);
    while (1) {
        static char msg[64];
        memset(msg, 0, sizeof(msg));
        printf("enter msg: \n");

        scanf_s("%s", msg, (int)sizeof(msg));
        int pkg_len;
        unsigned char* package = tp_protocol::package(
            (unsigned char*)msg, (int)strlen(msg), &pkg_len);
        send(s, (char*)package, pkg_len, 0);
        memset(msg, 0, sizeof(msg));
        tp_protocol::release_package((unsigned char*)package);

        int recved = recv(s, msg, sizeof(msg), 0);
        int pkg_size, header_size;
        tp_protocol::read_header((unsigned char*)msg, recved, &pkg_size,
                                 &header_size);
        printf("from server: %.*s . %d\n", pkg_size, msg + header_size, pkg_size);
    }

failed:
    if (s != INVALID_SOCKET) {
        closesocket(s);
        s = INVALID_SOCKET;
    }

#ifdef WIN32
    WSACleanup();
#endif
    // system("pause");
    return 0;
}
