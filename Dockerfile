FROM ubuntu:22.04
RUN apt-get update && apt-get install -y g++ make
WORKDIR /app
COPY include/ ./include/
COPY src/ ./src/
RUN g++ -std=c++17 -O2 -Iinclude \
    src/main.cpp src/TicketManager.cpp src/RequestQueue.cpp \
    src/ThreadPool.cpp src/Server.cpp \
    -o ticket_broker -lpthread
EXPOSE 8080
CMD ["./ticket_broker", "--server"]