#pragma once
#include <string>
#include <chrono>

enum class UserType { REGULAR = 0, VIP = 1 };

struct BookingRequest {
    int         userId;
    std::string userName;
    UserType    type;
    int         socketFd;   // fd to write response back; -1 for direct/test mode
    std::chrono::steady_clock::time_point arrivalTime;

    BookingRequest() : userId(0), type(UserType::REGULAR), socketFd(-1),
                       arrivalTime(std::chrono::steady_clock::now()) {}

    BookingRequest(int uid, const std::string& name, UserType t, int fd = -1)
        : userId(uid), userName(name), type(t), socketFd(fd),
          arrivalTime(std::chrono::steady_clock::now()) {}

    // Max-heap: VIP(1) > REGULAR(0); ties broken by earlier arrival
    bool operator<(const BookingRequest& other) const {
        if (this->type != other.type)
            return this->type < other.type;   // lower enum = lower priority
        return this->arrivalTime > other.arrivalTime; // earlier arrival wins
    }
};