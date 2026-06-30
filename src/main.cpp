#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include "TicketManager.h"
#include "RequestQueue.h"
#include "ThreadPool.h"
#include "Server.h"
#include "BookingRequest.h"

// ─────────────────────────────────────────────────────────────────────────────
//  USAGE:
//    ./ticket_broker                  → safe run  (mutex ON,  1000 users, 50 tickets)
//    ./ticket_broker --no-mutex       → chaos run (mutex OFF, shows double-bookings)
//    ./ticket_broker --server         → TCP mode  (listen on port 8080)
//    ./ticket_broker --server --no-mutex  → TCP mode without safety
// ─────────────────────────────────────────────────────────────────────────────

static const int TOTAL_TICKETS  = 50;
static const int TOTAL_USERS    = 1000;
static const int WORKER_THREADS = 8;
static const int SERVER_PORT    = 8080;

void runStressTest(bool useMutex) {
    std::cout << "\n";
    std::cout << "=================================================\n";
    std::cout << "  FLASH SALE TICKET BROKER — STRESS TEST\n";
    std::cout << "  Mode    : " << (useMutex ? "SAFE (mutex ON)" : "CHAOS (--no-mutex)") << "\n";
    std::cout << "  Tickets : " << TOTAL_TICKETS << "\n";
    std::cout << "  Users   : " << TOTAL_USERS   << "\n";
    std::cout << "  Workers : " << WORKER_THREADS << " pool threads\n";
    std::cout << "=================================================\n\n";

    TicketManager manager(TOTAL_TICKETS);
    RequestQueue  queue;
    ThreadPool    pool(WORKER_THREADS, queue, manager, useMutex);

    pool.start();

    // Build 1000 requests: ~20% VIP, 80% REGULAR
    std::mt19937 rng(42);
    std::vector<BookingRequest> requests;
    requests.reserve(TOTAL_USERS);
    for (int i = 1; i <= TOTAL_USERS; i++) {
        UserType t = (rng() % 5 == 0) ? UserType::VIP : UserType::REGULAR;
        std::string name = (t == UserType::VIP ? "VIPUser_" : "User_") + std::to_string(i);
        requests.emplace_back(i, name, t, -1 /* no socket */);
    }

    // Fire all 1000 requests as fast as possible from 1000 threads
    // (simulates all clients hitting at the exact same millisecond)
    std::vector<std::thread> senders;
    senders.reserve(TOTAL_USERS);
    for (auto& req : requests) {
        senders.emplace_back([&queue, &req]() {
            queue.push(req);
        });
    }
    for (auto& t : senders) t.join();   // wait for all pushes to complete

    // Give workers time to drain the queue
    while (queue.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // final flush

    pool.stop();
    manager.printSummary();
}

int main(int argc, char* argv[]) {
    bool useMutex  = true;
    bool serverMode = false;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--no-mutex")  useMutex   = false;
        if (arg == "--server")    serverMode  = true;
    }

    if (serverMode) {
        // TCP mode: ThreadPool workers + Server accept loop
        TicketManager manager(TOTAL_TICKETS);
        RequestQueue  queue;
        ThreadPool    pool(WORKER_THREADS, queue, manager, useMutex);
        pool.start();

        Server server(SERVER_PORT, queue);
        server.run();   // blocking

        pool.stop();
    } else {
        // Direct stress test mode (no networking needed)
        runStressTest(useMutex);
    }

    return 0;
}