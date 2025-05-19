#pragma once
#include <string>

class Server {
public:
    Server(int port);
    bool start();
    void stop();
    bool is_running() const;

private:
    int port_;
    bool running_;
}; 