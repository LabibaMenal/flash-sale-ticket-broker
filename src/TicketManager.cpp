#include "TicketManager.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <chrono>

TicketManager::TicketManager(int totalTickets)
    : availableTickets(totalTickets), totalTickets(totalTickets), nextTicketId(1) {}

int TicketManager::bookTicket(int userId, const std::string& userName, bool useMutex) {
    // ----- SAFE PATH (mutex locked) -----
    if (useMutex) {
        std::lock_guard<std::mutex> lock(mtx);
        if (availableTickets <= 0) return -1;
        int id = nextTicketId++;
        availableTickets--;
        bookings.push_back({id, userId, userName});
        return id;
    }

    // ----- UNSAFE PATH (no mutex — demo mode) -----
    // Artificial delay to amplify race window so corruption is visible
    if (availableTickets <= 0) return -1;
    int id = nextTicketId;          // two threads can read the same value here!
    // simulate a tiny bit of work between read and write
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    nextTicketId++;                 // non-atomic: two threads can both increment
    availableTickets--;             // non-atomic decrement — can underflow
    bookings.push_back({id, userId, userName});
    return id;
}

int TicketManager::getAvailable() const {
    std::lock_guard<std::mutex> lock(mtx);
    return availableTickets;
}

int TicketManager::getTotalBooked() const {
    std::lock_guard<std::mutex> lock(mtx);
    return static_cast<int>(bookings.size());
}

void TicketManager::printSummary() const {
    std::lock_guard<std::mutex> lock(mtx);
    std::cout << "\n========== BOOKING SUMMARY ==========\n";
    std::cout << "Total seats  : " << totalTickets     << "\n";
    std::cout << "Booked       : " << bookings.size()  << "\n";
    std::cout << "Available    : " << availableTickets  << "\n";

    // Detect duplicate ticket IDs (only possible in no-mutex mode)
    std::vector<int> ids;
    for (auto& b : bookings) ids.push_back(b.ticketId);
    std::sort(ids.begin(), ids.end());
    bool hasDups = std::adjacent_find(ids.begin(), ids.end()) != ids.end();
    std::cout << "Double-books : " << (hasDups ? "YES ← DATA CORRUPTION!" : "NONE (clean run)") << "\n";
    std::cout << "=====================================\n";
}