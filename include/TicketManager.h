#pragma once
#include <mutex>
#include <vector>
#include <string>

struct BookingRecord {
    int ticketId;
    int userId;
    std::string userName;
};

class TicketManager {
public:
    explicit TicketManager(int totalTickets);

    // Returns ticket ID (>=1) on success, -1 if sold out
    // useMutex=false triggers the race-condition demo
    int bookTicket(int userId, const std::string& userName, bool useMutex = true);

    int getAvailable()  const;
    int getTotalBooked() const;
    void printSummary()  const;

private:
    int availableTickets;
    int totalTickets;
    int nextTicketId;
    std::vector<BookingRecord> bookings;
    mutable std::mutex mtx;
};