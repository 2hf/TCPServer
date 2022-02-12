#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "54000"
WSADATA wsa_data;
SOCKET listening_socket = INVALID_SOCKET;
struct addrinfo* result = NULL;
struct addrinfo hints;

int server(int argc, char** argv) {

START:

    // initialize winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed");
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // resolve the server address and port
    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
   
    // create a socket for connecting to server
    listening_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

    // setup the tcp listening socket
    bind(listening_socket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    // flag this as the listening socket
    listen(listening_socket, SOMAXCONN);

    fd_set master;
    FD_ZERO(&master);

    FD_SET(listening_socket,&master);

    while (1) {
        fd_set master_copy;
        std::memcpy(&master_copy, &master, sizeof(fd_set));
        int socket_count = select(0, &master_copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < socket_count; ++i) {
            SOCKET sock = master_copy.fd_array[i];
            // accept a new connection
            if (sock == listening_socket) {
                SOCKET client = accept(listening_socket, nullptr, nullptr);
                FD_SET(client, &master);
            }
            // accept a new message
            else {
               
                char buf[4096];
                ZeroMemory(&buf, 4096);
                int received_bytes = recv(sock, buf, 4096, 0);
                std::cout << sock << " : " << buf;

                // invalid sock
                if (received_bytes <= 0 ) {
                    closesocket(sock);
                    FD_CLR(sock, &master);
                    std::cout << sock << " left" << std::endl;

                }
                else {
                    // relay message to all clients
                    for (int i = 0; i < master.fd_count; ++i) {
                        SOCKET out_socket = master.fd_array[i];
                        if (out_socket != listening_socket && out_socket != sock) {
                            std::ostringstream ss;
                            ss << sock << " : " << buf << "\r";
                            send(out_socket, ss.str().c_str(), ss.str().size() + 1, 0);
                        }
                    }
                }
            }
        }
    }

    // cleanup
    closesocket(listening_socket);
    WSACleanup();

    return 0;
}

int __cdecl main(int argc, char** argv)
{
    return server(argc, argv);
}
