#include "http_server.hpp"
#include "streaming.hpp"
#include <iostream>
#include <future>
#include <cstdlib>

HttpServer::HttpServer(ThreadSafeMediaQueue& queue) : media_queue_(queue) {
    // Read authentication token from environment variable
    const char* token_env = std::getenv("MYCHANNEL_AUTH_TOKEN");
    if (token_env) {
        auth_token_ = token_env;
        std::cout << "🔐 Authentication enabled with token" << std::endl;
    } else {
        std::cout << "⚠️ No MYCHANNEL_AUTH_TOKEN set - authentication disabled" << std::endl;
    }
    setup_routes();
}

bool HttpServer::is_authenticated(const httplib::Request& req) const {
    // If no token is configured, allow all requests (backward compatibility)
    if (auth_token_.empty()) {
        return true;
    }
    
    // Check for token in Authorization header (Bearer token format)
    auto auth_header = req.get_header_value("Authorization");
    if (!auth_header.empty()) {
        std::string expected = "Bearer " + auth_token_;
        if (auth_header == expected) {
            return true;
        }
    }
    
    // Check for token in query parameter
    if (req.has_param("token")) {
        std::string provided_token = req.get_param_value("token");
        if (provided_token == auth_token_) {
            return true;
        }
    }
    
    return false;
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
        std::cout << "🔍 DEBUG: POST /queue/add request received" << std::endl;
        std::cout << "   Method: " << req.method << std::endl;
        std::cout << "   Path: " << req.path << std::endl;
        std::cout << "   Query params count: " << req.params.size() << std::endl;
        for (const auto& param : req.params) {
            std::cout << "   Query param: " << param.first << " = " << param.second << std::endl;
        }
        std::cout << "   Headers count: " << req.headers.size() << std::endl;
        for (const auto& header : req.headers) {
            std::cout << "   Header: " << header.first << " = " << header.second << std::endl;
        }
        std::cout << "   Body: " << req.body << std::endl;
        
        if (!is_authenticated(req)) {
            std::cout << "   ❌ Authentication failed" << std::endl;
            res.status = 401;
            res.set_content("{\"status\":\"error\",\"message\":\"Authentication required\"}", "application/json");
            return;
        }
        std::cout << "   ✅ Authentication passed" << std::endl;
        
        if (req.has_param("url") || req.has_param("path")) {
            std::string item = req.has_param("url") ? req.get_param_value("url") : req.get_param_value("path");
            std::cout << "   ✅ Found parameter: " << item << std::endl;
            media_queue_.push(item);
            std::cout << "   ✅ Item added to queue: " << item << std::endl;
            res.set_content("{\"status\":\"success\",\"message\":\"Item added to queue\",\"item\":\"" + item + "\"}", "application/json");
        } else {
            std::cout << "   ❌ No url or path parameter found" << std::endl;
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"Missing url or path parameter\"}", "application/json");
        }
    });

    // POST /queue/priority - Add high-priority item to front of queue and interrupt current stream
    server_.Post("/queue/priority", [this](const httplib::Request& req, httplib::Response& res) {
        if (!is_authenticated(req)) {
            res.status = 401;
            res.set_content("{\"status\":\"error\",\"message\":\"Authentication required\"}", "application/json");
            return;
        }
        
        if (req.has_param("url") || req.has_param("path")) {
            std::string item = req.has_param("url") ? req.get_param_value("url") : req.get_param_value("path");
            
            // Add to front of queue
            media_queue_.push_front(item);
            
            // Request termination of current stream
            g_stream_process->request_termination();
            g_stream_process->kill_current_process();
            
            res.set_content("{\"status\":\"success\",\"message\":\"High-priority item added and current stream interrupted\",\"item\":\"" + item + "\"}", "application/json");
        } else {
            res.status = 400;
            res.set_content("{\"status\":\"error\",\"message\":\"Missing url or path parameter\"}", "application/json");
        }
    });

    // POST /queue/clear - Clear the queue
    server_.Post("/queue/clear", [this](const httplib::Request& req, httplib::Response& res) {
        if (!is_authenticated(req)) {
            res.status = 401;
            res.set_content("{\"status\":\"error\",\"message\":\"Authentication required\"}", "application/json");
            return;
        }
        
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
        if (!auth_token_.empty()) {
            std::cout << "🔐 Authentication is ENABLED - token required for write operations" << std::endl;
        } else {
            std::cout << "⚠️ Authentication is DISABLED - set MYCHANNEL_AUTH_TOKEN to enable" << std::endl;
        }
        std::cout << "Available endpoints:" << std::endl;
        std::cout << "  GET  /status - Server status (no auth required)" << std::endl;
        std::cout << "  GET  /queue - Get current queue (no auth required)" << std::endl;
        std::cout << "  POST /queue/add?url=<url>&token=<token> - Add URL to queue" << std::endl;
        std::cout << "  POST /queue/add?path=<path>&token=<token> - Add local file to queue" << std::endl;
        std::cout << "  POST /queue/priority?url=<url>&token=<token> - Add high-priority URL (interrupts current stream)" << std::endl;
        std::cout << "  POST /queue/priority?path=<path>&token=<token> - Add high-priority file (interrupts current stream)" << std::endl;
        std::cout << "  POST /queue/clear?token=<token> - Clear the queue" << std::endl;
        std::cout << "Alternative: Use Authorization: Bearer <token> header instead of token parameter" << std::endl;
        server_.listen(host, port);
    });
}

void HttpServer::stop() {
    server_.stop();
}
