#include "ThreadPool.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

// ─── ANALOGY ────────────────────────────────────────────────────────────────
// ThreadPool = a team of chefs (fixed count) who loop forever:
//   "grab next order → cook it → send plate → repeat"
// They don't go home between orders — that's the efficiency gain vs
// spawning a new thread per request (which is like hiring a new chef,
// training them, and firing them for every single dish).
// ─────────────────────────────────────────────────────────────────────────────

ThreadPool::ThreadPool(int n, RequestQueue& q, TicketManager& tm, bool mutex)
    : numThreads(n), useMutex(mutex), requestQueue(q), ticketManager(tm) {}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start() {
    running = true;
    for (int i = 0; i < numThreads; i++) {
        workers.emplace_back(&ThreadPool::workerLoop, this);
    }
}

void ThreadPool::stop() {
    running = false;
    requestQueue.shutdown();
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
    workers.clear();
}

void ThreadPool::workerLoop() {
    while (running) {
        BookingRequest req;
        if (!requestQueue.pop(req)) break;  // queue shut down, exit loop

        // Process the booking
        int ticketId = ticketManager.bookTicket(req.userId, req.userName, useMutex);

        // Build response string
        std::ostringstream oss;
        auto now = std::chrono::system_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()) % 1000;
        std::time_t t = std::chrono::system_clock::to_time_t(now);

        oss << "[" << std::put_time(std::localtime(&t), "%H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";

        if (ticketId > 0) {
            oss << "User " << std::setw(4) << req.userId
                << " [" << (req.type == UserType::VIP ? "VIP    " : "REGULAR") << "]"
                << " → BOOKED Ticket #" << std::setw(3) << ticketId;
        } else {
            oss << "User " << std::setw(4) << req.userId
                << " [" << (req.type == UserType::VIP ? "VIP    " : "REGULAR") << "]"
                << " → SOLD OUT";
        }
        std::string response = oss.str() + "\n";

        // Print to console always
        std::cout << response;

        // If request came from a TCP client, send response back over socket
        if (req.socketFd > 0) {
            send(req.socketFd, response.c_str(), response.size(), 0);
        }
    }
}