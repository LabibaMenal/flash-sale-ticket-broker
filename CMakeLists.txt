cmake_minimum_required(VERSION 3.14)
project(TicketBroker)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

add_executable(ticket_broker
    src/main.cpp
    src/TicketManager.cpp
    src/RequestQueue.cpp
    src/ThreadPool.cpp
    src/Server.cpp
)

target_include_directories(ticket_broker PRIVATE include)
target_link_libraries(ticket_broker Threads::Threads)

# Enable warnings
target_compile_options(ticket_broker PRIVATE -Wall -Wextra)