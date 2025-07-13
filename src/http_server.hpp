#pragma once
#include "media_queue.hpp"
#include <httplib.h>
#include <future>
#include <string>

class HttpServer {
public:
    httplib::Server server_;
    ThreadSafeMediaQueue& media_queue_;
    
    // Helper method to validate authentication
    bool is_authenticated(const httplib::Request& req) const;
    
    // Helper method to validate media items (files/URLs)
    bool is_valid_media_item(const std::string& item, std::string& error_message) const;
    
    explicit HttpServer(ThreadSafeMediaQueue& queue);
    void setup_routes();
    std::future<void> start_async(const std::string& host = "0.0.0.0", int port = 8080);
    void stop();

private:
    std::string auth_token_;
};
