#pragma once
#include <string>
#include "RequestQueue.h"

class Server {
public:
    Server(int port, RequestQueue& queue);
    ~Server();

    void run();   // blocking: accepts connections, parses commands, enqueues requests

private:
    void        handleClient(int clientSocket);
    BookingRequest parseCommand(const std::string& raw, int socketFd);

    int           port;
    int           serverFd;
    RequestQueue& requestQueue;
};