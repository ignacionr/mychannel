#pragma once
#include "media_queue.hpp"
#include <httplib.h>
#include <future>
#include <string>

class HttpServer {
private:
    httplib::Server server_;
    ThreadSafeMediaQueue& media_queue_;
    std::string auth_token_;
    
    // Helper method to validate authentication
    bool is_authenticated(const httplib::Request& req) const;

public:
    explicit HttpServer(ThreadSafeMediaQueue& queue);
    void setup_routes();
    std::future<void> start_async(const std::string& host = "0.0.0.0", int port = 8080);
    void stop();
};
