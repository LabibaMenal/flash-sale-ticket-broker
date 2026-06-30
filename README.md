# 🎫 Flash-Sale Concurrent Ticket Broker

A multithreaded C++17 ticket booking engine that simulates an **IRCTC Tatkal-style flash sale** - thousands of clients competing for a handful of seats at the exact same millisecond. Built to demonstrate OS-level concurrency concepts: mutex-protected shared state, condition-variable-based thread synchronization, priority scheduling, and raw TCP networking.

The core demo is a side-by-side comparison: same 1000 threads, same 50 seats, **one flag difference**.

| Mode | Result |
|---|---|
| Mutex ON (safe) | 50/50 booked, **zero double-bookings** |
| Mutex OFF (`--no-mutex`) | Duplicate ticket IDs, **memory corruption, program crash** |

---

## The Problem

Real-world flash sales (IRCTC Tatkal, concert ticket drops, e-commerce flash deals) face a brutal concurrency problem: thousands of users hit "Book" within the same millisecond for a tiny pool of resources. If the booking logic isn't properly synchronized, the same seat can be sold to multiple people - a race condition that causes real financial and reputational damage.

This project builds that exact scenario from scratch and proves, with a live before/after demo, why thread synchronization isn't optional.
I came accross Mutex and wondered how it can be applied for real life examples, and hence built this project to demo test that by building intentional broken demo codes and then running with on/off mutex.

---

## Architecture

```
TCP Client
    │  "BOOK_TICKET 42 Labiba VIP"
    ▼
Server            - accept loop; one lightweight thread per client just to read the command
    │  parses into BookingRequest struct
    ▼
RequestQueue      - thread-safe priority_queue + condition_variable
    │  VIP requests bubble to the top; pop() blocks workers until work is available
    ▼
ThreadPool        - 8 fixed worker threads, each looping forever
    │  calls bookTicket()
    ▼
TicketManager     - the shared resource; mutex-protected decrement + ID assignment
    │  returns ticketId or -1 (SOLD OUT)
    ▼
Response sent back to client (TCP mode) or printed to console (stress-test mode)
```

### Components

- **`BookingRequest`** - data packet for one booking attempt. Implements a custom `operator<` so VIP requests always outrank Regular ones in the priority queue, with earlier-arrival as the tiebreaker.
- **`TicketManager`** - owns the shared `availableTickets` counter. This is the *only* place tickets get decremented, and it's the only critical section protected by a mutex.
- **`RequestQueue`** - a thread-safe wrapper around `std::priority_queue`. Uses `std::condition_variable` so idle worker threads sleep at the OS level instead of busy-polling and wasting CPU.
- **`ThreadPool`** - a fixed pool of 8 worker threads created once at startup, avoiding the overhead of spawning a new OS thread for every single request.
- **`Server`** - the only networking-aware component. Raw TCP sockets via Winsock2, zero business logic - just reads commands and hands them to the queue.

---

## The Race Condition Demo

Run the exact same 1000-thread / 50-seat stress test twice:

**Safe run - mutex protects the critical section:**
```
.\ticket_broker.exe
```
![Safe run](assets/safe_run_1.png)
![Safe run summary](assets/safe_run_2.png)

Result: 50 tickets booked, VIPs consistently served first, **zero double-bookings**.

**Chaos run - mutex removed, artificial delay widens the race window:**
```
.\ticket_broker.exe --no-mutex
```
![Chaos run](assets/chaos_run_1.png)
![Chaos run duplicate IDs](assets/chaos_run_2.png)
![Chaos run crash](assets/chaos_run_3.png)

Result: the **same ticket ID gets handed out to multiple users simultaneously**, and the unsynchronized writes to the shared bookings list eventually corrupt memory and **crash the program outright**. This is the exact failure class that mutexes exist to prevent and why this was employed by me here.

---

## Why These Design Choices

| Decision | Why |
|---|---|
| **Thread pool over spawn-per-request** | Creating a new OS thread costs real time (~50-100µs) and 1000 simultaneous spawns cause massive context-switch overhead. A fixed pool of 8 workers reuses threads - the same pattern as Java's `ExecutorService`. |
| **`condition_variable` over busy-polling** | A polling loop checking "is there work yet?" burns 100% of a CPU core doing nothing. `cv.wait()` parks the thread at the OS level with zero CPU cost, and wakes instantly on `notify()`. |
| **Priority queue over plain FIFO** | Real flash-sale systems tier users (Tatkal premium, airline status tiers). A plain first-come-first-served queue misses this entirely. |
| **TCP over UDP** | A booking confirmation cannot be silently dropped. TCP's delivery and ordering guarantees are exactly what this problem needs. |
| **Raw sockets over a networking library** | Using Boost.Asio or similar would hide the OS-level mechanics (bind, listen, accept) that this project is meant to demonstrate. |
| **Zero external dependencies** | Every `#include` in this project is one I can explain line by line. |

---

## Build & Run

**Requirements:** C++17 compiler (tested with w64devkit / MinGW g++ on Windows 11)

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp src/TicketManager.cpp src/RequestQueue.cpp src/ThreadPool.cpp src/Server.cpp -o ticket_broker -lws2_32
```

**Run modes:**
```bash
.\ticket_broker.exe              # stress test: 1000 users, 50 seats, mutex ON
.\ticket_broker.exe --no-mutex   # same test, mutex OFF - watch it break
.\ticket_broker.exe --server     # TCP server mode, listens on port 8080
```

**TCP protocol** (when running `--server`):
```
BOOK_TICKET <userId> <userName> <VIP|REGULAR>
```

---

## Project Structure

```
ticket-broker/
├── include/
│   ├── BookingRequest.h
│   ├── TicketManager.h
│   ├── RequestQueue.h
│   ├── ThreadPool.h
│   └── Server.h
└── src/
    ├── main.cpp
    ├── TicketManager.cpp
    ├── RequestQueue.cpp
    ├── ThreadPool.cpp
    └── Server.cpp
```

---

## What I Learned

Building the unsafe path alongside the safe one was more valuable than just implementing the "correct" version - it forced me to understand *exactly* what a mutex prevents at the memory level, not just that I should "use a mutex because the document says so." Watching the program corrupt its own memory and crash made the abstract concept of a race condition concrete in a way that reading about it never did and also made me utilise my core c++ knowledge here.