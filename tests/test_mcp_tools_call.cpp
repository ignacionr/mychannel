#include <gtest/gtest.h>
#include <glaze/glaze.hpp>
#include "../src/mcp_server.hpp"
#include "../src/http_server.hpp"
#include "../src/media_queue.hpp"

class MCPToolsCallTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue = std::make_unique<ThreadSafeMediaQueue>();
        http_server = std::make_unique<HttpServer>(*queue);
        mcp_server = std::make_unique<MCPServer>(*http_server);
    }

    std::unique_ptr<ThreadSafeMediaQueue> queue;
    std::unique_ptr<HttpServer> http_server;
    std::unique_ptr<MCPServer> mcp_server;
};

// Test the exact JSON-RPC 2.0 request that was failing
TEST_F(MCPToolsCallTest, ExactFailingRequest) {
    std::string json = R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":"test-123"})";
    
    // Parse the request
    auto json_request = mcp_server->parse_json(json);
    
    // Verify basic structure
    ASSERT_TRUE(json_request.contains("jsonrpc"));
    EXPECT_EQ(json_request["jsonrpc"].get_string(), "2.0");
    
    ASSERT_TRUE(json_request.contains("method"));
    EXPECT_EQ(json_request["method"].get_string(), "tools/call");
    
    ASSERT_TRUE(json_request.contains("id"));
    EXPECT_EQ(json_request["id"].get_string(), "test-123");
    
    // Verify params structure
    ASSERT_TRUE(json_request.contains("params"));
    ASSERT_TRUE(json_request["params"].is_object());
    
    auto& params = json_request["params"].get_object();
    ASSERT_TRUE(params.contains("name"));
    EXPECT_EQ(params.at("name").get_string(), "get_streaming_queue");
    
    ASSERT_TRUE(params.contains("arguments"));
    ASSERT_TRUE(params.at("arguments").is_object());
    
    // Test arguments serialization
    auto arguments_dump = params.at("arguments").dump();
    ASSERT_TRUE(arguments_dump.has_value());
    std::string arguments_json = *arguments_dump;
    EXPECT_EQ(arguments_json, "{}");
}

// Test tools/call with actual arguments
TEST_F(MCPToolsCallTest, ToolsCallWithRealArguments) {
    std::string json = R"({
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {
            "name": "add_video_to_queue",
            "arguments": {
                "source": "videos/test.mp4",
                "position": "end"
            }
        },
        "id": "call-456"
    })";
    
    auto json_request = mcp_server->parse_json(json);
    auto& params = json_request["params"].get_object();
    auto& arguments = params.at("arguments").get_object();
    
    // Verify argument extraction
    ASSERT_TRUE(arguments.contains("source"));
    EXPECT_EQ(arguments.at("source").get_string(), "videos/test.mp4");
    
    ASSERT_TRUE(arguments.contains("position"));
    EXPECT_EQ(arguments.at("position").get_string(), "end");
    
    // Test serialization back to JSON
    auto arguments_dump = params.at("arguments").dump();
    ASSERT_TRUE(arguments_dump.has_value());
    std::string arguments_json = *arguments_dump;
    
    // Parse the serialized JSON to verify it's valid
    auto reparsed = mcp_server->parse_json(arguments_json);
    ASSERT_TRUE(reparsed.contains("source"));
    EXPECT_EQ(reparsed["source"].get_string(), "videos/test.mp4");
}

// Test the handle_mcp_tool_call function directly
TEST_F(MCPToolsCallTest, HandleMcpToolCallDirect) {
    std::string json = R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":"test-123"})";
    
    auto json_request = mcp_server->parse_json(json);
    
    // This should not throw an exception
    EXPECT_NO_THROW({
        std::string result = mcp_server->handle_mcp_tool_call("test-123", json_request);
        
        // The result should be a valid JSON-RPC 2.0 response
        EXPECT_TRUE(result.find("\"jsonrpc\":\"2.0\"") != std::string::npos);
        EXPECT_TRUE(result.find("\"id\":\"test-123\"") != std::string::npos);
        // Should either have "result" or "error" field
        EXPECT_TRUE(result.find("\"result\":") != std::string::npos || 
                   result.find("\"error\":") != std::string::npos);
    });
}

// Test ID extraction with different types
TEST_F(MCPToolsCallTest, IdExtractionTypes) {
    // String ID
    std::string json_str_id = R"({"jsonrpc":"2.0","method":"tools/list","id":"string-id"})";
    auto result_str = mcp_server->parse_json(json_str_id);
    ASSERT_TRUE(result_str.contains("id"));
    ASSERT_TRUE(result_str["id"].is_string());
    EXPECT_EQ(result_str["id"].get_string(), "string-id");
    
    // Numeric ID
    std::string json_num_id = R"({"jsonrpc":"2.0","method":"tools/list","id":42})";
    auto result_num = mcp_server->parse_json(json_num_id);
    ASSERT_TRUE(result_num.contains("id"));
    ASSERT_TRUE(result_num["id"].is_number());
    EXPECT_EQ(result_num["id"].get_number(), 42.0);
    
    // Null ID
    std::string json_null_id = R"({"jsonrpc":"2.0","method":"tools/list","id":null})";
    auto result_null = mcp_server->parse_json(json_null_id);
    ASSERT_TRUE(result_null.contains("id"));
    ASSERT_TRUE(result_null["id"].is_null());
}
