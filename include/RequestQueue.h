#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include "BookingRequest.h"

class RequestQueue {
public:
    void push(const BookingRequest& req);

    // Blocks the calling thread until an item is available or queue is shut down
    // Returns false if queue is shut down and empty (signals worker to exit)
    bool pop(BookingRequest& out);

    void shutdown();   // wake all waiting workers so they can exit cleanly
    int  size() const;

private:
    std::priority_queue<BookingRequest> pq;
    mutable std::mutex              mtx;
    std::condition_variable         cv;
    bool                            done = false;
};