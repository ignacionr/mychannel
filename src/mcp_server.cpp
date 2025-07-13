#include "mcp_server.hpp"
#include "utils.hpp"
#include "media_info.hpp"
#include "streaming.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <glaze/glaze.hpp>

MCPServer::MCPServer(HttpServer& server) : http_server_(server) {
    // Initialize available MCP tools with simpler JSON schemas
    tools_ = {
        {
            "add_video_to_queue",
            "Add a video (YouTube URL or local file path) to the streaming queue",
            "{\"type\":\"object\",\"properties\":{\"source\":{\"type\":\"string\"},\"position\":{\"type\":\"string\"}},\"required\":[\"source\"]}"
        },
        {
            "add_priority_video", 
            "Add high-priority video that interrupts current stream immediately",
            "{\"type\":\"object\",\"properties\":{\"source\":{\"type\":\"string\"},\"reason\":{\"type\":\"string\"}},\"required\":[\"source\"]}"
        },
        {
            "get_streaming_queue",
            "Get current streaming queue status and contents", 
            "{\"type\":\"object\",\"properties\":{}}"
        },
        {
            "clear_streaming_queue",
            "Clear the entire streaming queue",
            "{\"type\":\"object\",\"properties\":{}}"
        },
        {
            "get_stream_status",
            "Get current streaming status and progress information",
            "{\"type\":\"object\",\"properties\":{}}"
        },
        {
            "interrupt_current_stream", 
            "Immediately interrupt the current stream",
            "{\"type\":\"object\",\"properties\":{\"reason\":{\"type\":\"string\"}},\"required\":[]}"
        },
        {
            "get_video_duration",
            "Get duration of a video file or YouTube URL",
            "{\"type\":\"object\",\"properties\":{\"source\":{\"type\":\"string\"}},\"required\":[\"source\"]}"
        },
        {
            "validate_video_source",
            "Check if video source is accessible and valid",
            "{\"type\":\"object\",\"properties\":{\"source\":{\"type\":\"string\"}},\"required\":[\"source\"]}"
        }
    };
    
    setup_mcp_routes();
}

void MCPServer::setup_mcp_routes() {
    // MCP JSON-RPC 2.0 endpoint
    http_server_.server_.Post("/", [this](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Content-Type", "application/json");
        
        try {
            // Parse JSON-RPC request with Glaze
            glz::json_t json_request;
            auto parse_error = glz::read_json(json_request, req.body);
            
            if (parse_error) {
                std::string error_response = create_mcp_error_response("null", -32700, "Parse error");
                res.set_content(error_response, "application/json");
                return;
            }
            
            // Extract method and id from JSON
            std::string method;
            std::string id;
            
            if (json_request.contains("method") && json_request["method"].is_string()) {
                method = json_request["method"].get_string();
            }
            
            if (json_request.contains("id")) {
                if (json_request["id"].is_string()) {
                    id = json_request["id"].get_string();
                } else if (json_request["id"].is_number()) {
                    id = std::to_string(json_request["id"].get_number());
                }
            }
            
            std::string response;
            
            if (method == "initialize") {
                response = handle_mcp_initialize(id);
            } else if (method == "tools/list") {
                response = handle_mcp_tools_list(id);
            } else if (method == "tools/call") {
                if (!http_server_.is_authenticated(req)) {
                    response = create_mcp_error_response(id, -32001, "Authentication required");
                } else {
                    response = handle_mcp_tool_call(id, json_request);
                }
            } else {
                response = create_mcp_error_response(id, -32601, "Method not found");
            }
            
            res.set_content(response, "application/json");
        } catch (const std::exception& e) {
            std::string error_response = create_mcp_error_response("null", -32700, "Parse error");
            res.set_content(error_response, "application/json");
        }
    });

    // Legacy endpoints for backward compatibility
    // MCP endpoint for tool discovery
    http_server_.server_.Get("/mcp/tools", [this](const httplib::Request&, httplib::Response& res) {
        std::string json_response = get_tools_schema();
        res.set_content(json_response, "application/json");
    });
    
    // MCP endpoint for tool execution
    http_server_.server_.Post("/mcp/call", [this](const httplib::Request& req, httplib::Response& res) {
        if (!http_server_.is_authenticated(req)) {
            res.status = 401;
            res.set_content(create_error_response("Authentication required"), "application/json");
            return;
        }
        
        // Parse the main request body with Glaze
        auto main_json = parse_json(req.body);
        std::string tool_name;
        
        if (main_json.contains("tool") && main_json["tool"].is_string()) {
            tool_name = main_json["tool"].get_string();
        }
        
        // For parameters, we need to extract the params object from the JSON
        // This is a simplified approach - extract parameters from the main JSON
        std::string result;
        
        // Route to appropriate tool handler - pass the original body for param extraction
        if (tool_name == "add_video_to_queue") {
            result = handle_add_video_to_queue(req.body);
        } else if (tool_name == "add_priority_video") {
            result = handle_add_priority_video(req.body);
        } else if (tool_name == "get_streaming_queue") {
            result = handle_get_streaming_queue(req.body);
        } else if (tool_name == "clear_streaming_queue") {
            result = handle_clear_streaming_queue(req.body);
        } else if (tool_name == "get_stream_status") {
            result = handle_get_stream_status(req.body);
        } else if (tool_name == "interrupt_current_stream") {
            result = handle_interrupt_current_stream(req.body);
        } else if (tool_name == "get_video_duration") {
            result = handle_get_video_duration(req.body);
        } else if (tool_name == "validate_video_source") {
            result = handle_validate_video_source(req.body);
        } else {
            result = create_error_response("Unknown tool: " + tool_name);
        }
        
        res.set_content(result, "application/json");
    });
}

std::string MCPServer::get_tools_schema() const {
    std::ostringstream oss;
    oss << "{\"tools\":[";
    for (size_t i = 0; i < tools_.size(); ++i) {
        oss << "{\"name\":\"" << tools_[i].name << "\",";
        oss << "\"description\":\"" << tools_[i].description << "\",";
        oss << "\"inputSchema\":" << tools_[i].input_schema << "}";
        if (i < tools_.size() - 1) oss << ",";
    }
    oss << "]}";
    return oss.str();
}

// Tool implementations using existing functionality
std::string MCPServer::handle_add_video_to_queue(const std::string& full_request) {
    auto parsed = extract_mcp_params(full_request);
    auto source = parsed["source"];
    auto position = parsed["position"];
    
    if (source.empty()) {
        return create_error_response("Missing required parameter: source");
    }
    
    try {
        if (position == "front") {
            http_server_.media_queue_.push_front(source);
        } else {
            http_server_.media_queue_.push(source);
        }
        return create_success_response("\"Video added to queue: " + source + "\"");
    } catch (const std::exception& e) {
        return create_error_response("Failed to add video: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_add_priority_video(const std::string& full_request) {
    auto parsed = extract_mcp_params(full_request);
    auto source = parsed["source"];
    auto reason = parsed["reason"];
    
    if (source.empty()) {
        return create_error_response("Missing required parameter: source");
    }
    
    try {
        // Add to front of queue
        http_server_.media_queue_.push_front(source);
        
        // Interrupt current stream
        g_stream_process->request_termination();
        g_stream_process->kill_current_process();
        
        std::string msg = "\"Priority video added and current stream interrupted: " + source;
        if (!reason.empty()) {
            msg += " (Reason: " + reason + ")";
        }
        msg += "\"";
        return create_success_response(msg);
    } catch (const std::exception& e) {
        return create_error_response("Failed to add priority video: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_get_streaming_queue(const std::string&) {
    try {
        auto items = http_server_.media_queue_.get_all_items();
        std::ostringstream oss;
        oss << "{\"queue\":[";
        for (size_t i = 0; i < items.size(); ++i) {
            oss << "\"" << items[i] << "\"";
            if (i < items.size() - 1) oss << ",";
        }
        oss << "],\"size\":" << items.size();
        oss << ",\"is_streaming\":" << (!g_stream_process->should_terminate() ? "true" : "false");
        oss << "}";
        return create_success_response(oss.str());
    } catch (const std::exception& e) {
        return create_error_response("Failed to get queue: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_clear_streaming_queue(const std::string&) {
    try {
        http_server_.media_queue_.clear();
        return create_success_response("\"Queue cleared successfully\"");
    } catch (const std::exception& e) {
        return create_error_response("Failed to clear queue: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_get_stream_status(const std::string&) {
    try {
        auto items = http_server_.media_queue_.get_all_items();
        bool is_streaming = !g_stream_process->should_terminate();
        
        std::ostringstream oss;
        oss << "{\"is_streaming\":" << (is_streaming ? "true" : "false");
        oss << ",\"queue_size\":" << items.size();
        oss << ",\"fallback_video\":\"videos/News_Intro.mp4\"";
        oss << ",\"server_status\":\"running\"";
        oss << "}";
        return create_success_response(oss.str());
    } catch (const std::exception& e) {
        return create_error_response("Failed to get status: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_interrupt_current_stream(const std::string& full_request) {
    auto parsed = extract_mcp_params(full_request);
    auto reason = parsed["reason"];
    
    try {
        g_stream_process->request_termination();
        g_stream_process->kill_current_process();
        
        std::string msg = "\"Current stream interrupted";
        if (!reason.empty()) {
            msg += " (Reason: " + reason + ")";
        }
        msg += "\"";
        return create_success_response(msg);
    } catch (const std::exception& e) {
        return create_error_response("Failed to interrupt stream: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_get_video_duration(const std::string& full_request) {
    auto parsed = extract_mcp_params(full_request);
    auto source = parsed["source"];
    
    if (source.empty()) {
        return create_error_response("Missing required parameter: source");
    }
    
    try {
        double duration;
        if (is_youtube_url(source)) {
            duration = get_youtube_duration(source);
        } else {
            duration = get_media_duration(source);
        }
        
        std::ostringstream oss;
        oss << "{\"duration\":" << duration << ",\"source\":\"" << source << "\"}";
        return create_success_response(oss.str());
    } catch (const std::exception& e) {
        return create_error_response("Failed to get duration: " + std::string(e.what()));
    }
}

std::string MCPServer::handle_validate_video_source(const std::string& full_request) {
    auto parsed = extract_mcp_params(full_request);
    auto source = parsed["source"];
    
    if (source.empty()) {
        return create_error_response("Missing required parameter: source");
    }
    
    try {
        bool is_valid = false;
        std::string source_type;
        
        if (is_youtube_url(source)) {
            source_type = "youtube";
            // Try to get duration as validation
            double duration = get_youtube_duration(source);
            is_valid = (duration > 0);
        } else {
            source_type = "local_file";
            // Try to get duration as validation  
            double duration = get_media_duration(source);
            is_valid = (duration > 0);
        }
        
        std::ostringstream oss;
        oss << "{\"is_valid\":" << (is_valid ? "true" : "false");
        oss << ",\"source_type\":\"" << source_type << "\"";
        oss << ",\"source\":\"" << source << "\"}";
        return create_success_response(oss.str());
    } catch (const std::exception& e) {
        return create_error_response("Failed to validate source: " + std::string(e.what()));
    }
}

// MCP JSON-RPC 2.0 protocol implementations
std::string MCPServer::handle_mcp_initialize(const std::string& id) {
    return R"({"jsonrpc":"2.0","id":")" + id + R"(","result":{"protocolVersion":"2024-11-05","capabilities":{"tools":{},"resources":{},"prompts":{},"logging":{}},"serverInfo":{"name":"mychannel","version":"1.0.0"}}})";
}

std::string MCPServer::handle_mcp_tools_list(const std::string& id) {
    std::ostringstream oss;
    oss << R"({"jsonrpc":"2.0","id":")" << id << R"(","result":{"tools":[)";
    
    for (size_t i = 0; i < tools_.size(); ++i) {
        oss << R"({"name":")" << tools_[i].name << R"(",)";
        oss << R"("description":")" << tools_[i].description << R"(",)";
        oss << R"("inputSchema":)" << tools_[i].input_schema << "}";
        if (i < tools_.size() - 1) oss << ",";
    }
    
    oss << "]}}";
    return oss.str();
}

std::string MCPServer::handle_mcp_tool_call(const std::string& id, const glz::json_t& request) {
    try {
        // Extract tool name and arguments from MCP request
        std::string tool_name;
        std::string params_json;
        
        // Navigate to params.name
        if (request.contains("params") && request["params"].is_object()) {
            const auto& params = request["params"].get_object();
            
            if (params.contains("name") && params.at("name").is_string()) {
                tool_name = params.at("name").get_string();
            }
            
            if (params.contains("arguments") && params.at("arguments").is_object()) {
                // Convert arguments object back to JSON string for tool handlers
                auto arguments_dump = params.at("arguments").dump();
                if (arguments_dump) {
                    params_json = *arguments_dump;
                }
            }
        }
        
        if (tool_name.empty()) {
            return create_mcp_error_response(id, -32602, "Invalid params: missing tool name");
        }
        
        std::string result;
        
        // Route to appropriate tool handler
        if (tool_name == "add_video_to_queue") {
            result = handle_add_video_to_queue(params_json);
        } else if (tool_name == "add_priority_video") {
            result = handle_add_priority_video(params_json);
        } else if (tool_name == "get_streaming_queue") {
            result = handle_get_streaming_queue(params_json);
        } else if (tool_name == "clear_streaming_queue") {
            result = handle_clear_streaming_queue(params_json);
        } else if (tool_name == "get_stream_status") {
            result = handle_get_stream_status(params_json);
        } else if (tool_name == "interrupt_current_stream") {
            result = handle_interrupt_current_stream(params_json);
        } else if (tool_name == "get_video_duration") {
            result = handle_get_video_duration(params_json);
        } else if (tool_name == "validate_video_source") {
            result = handle_validate_video_source(params_json);
        } else {
            return create_mcp_error_response(id, -32601, "Tool not found: " + tool_name);
        }
        
        return R"({"jsonrpc":"2.0","id":")" + id + R"(","result":{"content":[{"type":"text","text":)" + result + "}]}}";
    } catch (const std::exception& e) {
        return create_mcp_error_response(id, -32602, "Invalid params: " + std::string(e.what()));
    }
}

std::string MCPServer::create_mcp_error_response(const std::string& id, int code, const std::string& message) {
    return R"({"jsonrpc":"2.0","id":")" + id + R"(","error":{"code":)" + std::to_string(code) + R"(,"message":")" + message + R"("}})";
}

// Helper methods
std::string MCPServer::create_error_response(const std::string& error_msg) {
    return "{\"status\":\"error\",\"message\":\"" + error_msg + "\"}";
}

std::string MCPServer::create_success_response(const std::string& result) {
    return "{\"status\":\"success\",\"result\":" + result + "}";
}

glz::json_t MCPServer::parse_json(const std::string& json) {
    glz::json_t result;
    auto parse_error = glz::read_json(result, json);
    if (parse_error) {
        throw std::runtime_error("JSON parse error");
    }
    return result;
}

// Helper to extract parameters from nested params object in MCP call
std::map<std::string, std::string> MCPServer::extract_mcp_params(const std::string& json) {
    std::map<std::string, std::string> result;
    
    try {
        auto json_obj = parse_json(json);
        
        if (json_obj.contains("params") && json_obj["params"].is_object()) {
            const auto& params = json_obj["params"].get_object();
            
            // Extract string parameters
            for (const auto& [key, value] : params) {
                if (value.is_string()) {
                    result[key] = value.get_string();
                } else if (value.is_number()) {
                    result[key] = std::to_string(value.get_number());
                } else if (value.is_boolean()) {
                    result[key] = value.get<bool>() ? "true" : "false";
                }
            }
        }
    } catch (const std::exception&) {
        // Return empty result on parse error
    }
    
    return result;
}

std::vector<MCPTool> MCPServer::get_available_tools() const {
    return tools_;
}
