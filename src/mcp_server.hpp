#pragma once
#include "http_server.hpp"
#include <string>
#include <vector>
#include <map>

// MCP Tool definition structure
struct MCPTool {
    std::string name;
    std::string description;
    std::string input_schema;  // JSON schema as string
};

// MCP Server extension for HttpServer
class MCPServer {
private:
    HttpServer& http_server_;
    std::vector<MCPTool> tools_;
    
    // Tool implementations
    std::string handle_add_video_to_queue(const std::string& params);
    std::string handle_add_priority_video(const std::string& params);
    std::string handle_get_streaming_queue(const std::string& params);
    std::string handle_clear_streaming_queue(const std::string& params);
    std::string handle_get_stream_status(const std::string& params);
    std::string handle_interrupt_current_stream(const std::string& params);
    std::string handle_get_video_duration(const std::string& params);
    std::string handle_validate_video_source(const std::string& params);
    
    // Helper methods
    std::string create_error_response(const std::string& error_msg);
    std::string create_success_response(const std::string& result);
    std::map<std::string, std::string> parse_json_params(const std::string& json);
    std::map<std::string, std::string> extract_mcp_params(const std::string& json);

public:
    explicit MCPServer(HttpServer& server);
    void setup_mcp_routes();
    std::vector<MCPTool> get_available_tools() const;
    std::string get_tools_schema() const;
};
