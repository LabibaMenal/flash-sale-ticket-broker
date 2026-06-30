#include "RequestQueue.h"

// ─── ANALOGY ────────────────────────────────────────────────────────────────
// This queue is like a restaurant's order rail with a VIP section.
// push()  → kitchen staff pins a new order to the rail and rings the bell
// pop()   → a chef grabs the highest-priority order; sleeps if rail is empty
// shutdown() → manager rings the closing bell so all chefs stop waiting
// ─────────────────────────────────────────────────────────────────────────────

void RequestQueue::push(const BookingRequest& req) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        pq.push(req);
    }
    cv.notify_one();   // wake one sleeping worker thread
}

bool RequestQueue::pop(BookingRequest& out) {
    std::unique_lock<std::mutex> lock(mtx);

    // Sleep until: (a) there's something to process, OR (b) we're shutting down
    cv.wait(lock, [this]() { return !pq.empty() || done; });

    if (pq.empty()) return false;   // done=true and queue empty → worker should exit

    out = pq.top();
    pq.pop();
    return true;
}

void RequestQueue::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();   // wake ALL sleeping workers so they can check done=true and exit
}

int RequestQueue::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return static_cast<int>(pq.size());
}