#pragma once
#include <vector>
#include <thread>
#include <atomic>
#include "RequestQueue.h"
#include "TicketManager.h"

class ThreadPool {
public:
    ThreadPool(int numThreads, RequestQueue& queue, TicketManager& manager, bool useMutex);
    ~ThreadPool();

    void start();
    void stop();

private:
    void workerLoop();   // each thread runs this in a loop until shutdown

    int            numThreads;
    bool           useMutex;
    std::atomic<bool> running{false};
    std::vector<std::thread> workers;
    RequestQueue&  requestQueue;
    TicketManager& ticketManager;
};