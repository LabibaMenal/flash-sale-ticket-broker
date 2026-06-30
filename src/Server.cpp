#include "Server.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

Server::Server(int port, RequestQueue& queue)
    : port(port), serverFd(-1), requestQueue(queue) {}

Server::~Server() {
#ifdef _WIN32
    if (serverFd > 0) closesocket(serverFd);
    WSACleanup();
#else
    if (serverFd > 0) close(serverFd);
#endif
}

void Server::run() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return; }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);
    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return; }

    listen(serverFd, 128);
    std::cout << "[Server] Listening on port " << port << " ...\n";
    std::cout << "[Server] Send: BOOK_TICKET <id> <name> <VIP|REGULAR>\n\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t len = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &len);
        if (clientFd < 0) break;
        std::thread([this, clientFd]() { handleClient(clientFd); }).detach();
    }
}

void Server::handleClient(int clientSocket) {
    char buf[256] = {};
    int n = recv(clientSocket, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
        return;
    }

    std::string raw(buf, n);
    while (!raw.empty() && (raw.back() == '\n' || raw.back() == '\r'))
        raw.pop_back();

    BookingRequest req = parseCommand(raw, clientSocket);
    requestQueue.push(req);
}

BookingRequest Server::parseCommand(const std::string& raw, int socketFd) {
    std::istringstream iss(raw);
    std::string cmd, name, typeStr;
    int uid = 0;
    iss >> cmd >> uid >> name >> typeStr;

    UserType t = (typeStr == "VIP") ? UserType::VIP : UserType::REGULAR;
    return BookingRequest(uid, name, t, socketFd);
}