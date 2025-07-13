#include "http_server.hpp"
#include <iostream>
#include <future>

HttpServer::HttpServer(ThreadSafeMediaQueue& queue) : media_queue_(queue) {
    setup_routes();
}

void HttpServer::setup_routes() {
    // CORS headers for all responses
    server_.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });

    // Handle OPTIONS requests for CORS preflight
    server_.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return;
    });

    // GET /queue - Get current queue status
    server_.Get("/queue", [this](const httplib::Request&, httplib::Response& res) {
        auto items = media_queue_.get_all_items();
        std::string json_response = "{\"queue\":[";
        for (size_t i = 0; i < items.size(); ++i) {
            json_response += "\"" + items[i] + "\"";
            if (i < items.size() - 1) json_response += ",";
        }
        json_response += "],\"size\":" + std::to_string(items.size()) + "}";
        res.set_content(json_response, "application/json");
    });

    // POST /queue/add - Add item to queue
    server_.Post("/queue/add", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.has_param("url") || req.has_param("path")) {
            std::string item = req.has_param("url") ? req.get_param_value("url") : req.get_param_value("path");
            media_queue_.push(item);
            res.set_content("{\"status\":\"success\",\"message\":\"Item added to queue\",\"item\":\"" + item + "\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"Missing url or path parameter\"}", "application/json");
        }
    });

    // POST /queue/clear - Clear the queue
    server_.Post("/queue/clear", [this](const httplib::Request&, httplib::Response& res) {
        media_queue_.clear();
        res.set_content("{\"status\":\"success\",\"message\":\"Queue cleared\"}", "application/json");
    });

    // GET /status - Get server status
    server_.Get("/status", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"running\",\"server\":\"mychannel\",\"fallback_video\":\"videos/News_Intro.mp4\"}", "application/json");
    });
}

std::future<void> HttpServer::start_async(const std::string& host, int port) {
    return std::async(std::launch::async, [this, host, port]() {
        std::cout << "Starting HTTP server on http://" << host << ":" << port << std::endl;
        std::cout << "Available endpoints:" << std::endl;
        std::cout << "  GET  /status - Server status" << std::endl;
        std::cout << "  GET  /queue - Get current queue" << std::endl;
        std::cout << "  POST /queue/add?url=<url> - Add URL to queue" << std::endl;
        std::cout << "  POST /queue/add?path=<path> - Add local file to queue" << std::endl;
        std::cout << "  POST /queue/clear - Clear the queue" << std::endl;
        server_.listen(host, port);
    });
}

void HttpServer::stop() {
    server_.stop();
}
