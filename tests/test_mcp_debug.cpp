#include <gtest/gtest.h>
#include <glaze/glaze.hpp>
#include "../src/mcp_server.hpp"
#include "../src/http_server.hpp"
#include "../src/media_queue.hpp"

class MCPDebugTest : public ::testing::Test {
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

// Test the exact flow that happens in the server
TEST_F(MCPDebugTest, SimulateServerFlow) {
    // This is the exact JSON that was failing
    std::string request_body = R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":"test-123"})";
    
    std::cout << "Request body: " << request_body << std::endl;
    std::cout << "Request length: " << request_body.length() << std::endl;
    
    try {
        // Step 1: Parse JSON-RPC request
        glz::json_t json_request;
        auto parse_error = glz::read_json(json_request, request_body);
        
        ASSERT_FALSE(parse_error) << "JSON parsing failed";
        std::cout << "✓ JSON parsing successful" << std::endl;
        
        // Step 2: Extract method and id
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
        
        std::cout << "Method: " << method << std::endl;
        std::cout << "ID: " << id << std::endl;
        
        EXPECT_EQ(method, "tools/call");
        EXPECT_EQ(id, "test-123");
        
        // Step 3: Call the tool handler
        if (method == "tools/call") {
            std::string response = mcp_server->handle_mcp_tool_call(id, json_request);
            std::cout << "Response: " << response << std::endl;
            
            // Verify the response is valid JSON
            auto response_json = mcp_server->parse_json(response);
            ASSERT_TRUE(response_json.contains("jsonrpc"));
            ASSERT_TRUE(response_json.contains("id"));
            
            // Should have either result or error
            bool has_result = response_json.contains("result");
            bool has_error = response_json.contains("error");
            EXPECT_TRUE(has_result || has_error);
            
            if (has_result) {
                std::cout << "✓ Success response received" << std::endl;
            } else if (has_error) {
                std::cout << "✗ Error response: " << response << std::endl;
                auto& error = response_json["error"].get_object();
                if (error.contains("message")) {
                    std::cout << "Error message: " << error.at("message").get_string() << std::endl;
                }
            }
        }
        
    } catch (const std::exception& e) {
        FAIL() << "Exception thrown: " << e.what();
    }
}

// Test with various malformed JSON to see error handling
TEST_F(MCPDebugTest, MalformedJsonHandling) {
    std::vector<std::string> malformed_cases = {
        R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{},"id":"test-123"})",  // Missing closing brace
        R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":"test-123",})", // Trailing comma
        R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":})", // Missing id value
        R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue"},"id":"test-123"})", // Missing arguments
    };
    
    for (size_t i = 0; i < malformed_cases.size(); i++) {
        std::cout << "Testing malformed case " << i + 1 << ": " << malformed_cases[i] << std::endl;
        
        try {
            glz::json_t json_request;
            auto parse_error = glz::read_json(json_request, malformed_cases[i]);
            
            if (parse_error) {
                std::cout << "✓ Parse error correctly detected for case " << i + 1 << std::endl;
            } else {
                std::cout << "? Parse succeeded for case " << i + 1 << " (might be valid JSON)" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✓ Exception correctly thrown for case " << i + 1 << ": " << e.what() << std::endl;
        }
    }
}

// Test edge cases with different ID types
TEST_F(MCPDebugTest, IdTypesInResponse) {
    std::vector<std::pair<std::string, std::string>> test_cases = {
        {R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":"string-id"})", "string-id"},
        {R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":42})", "42"},
        {R"({"jsonrpc":"2.0","method":"tools/call","params":{"name":"get_streaming_queue","arguments":{}},"id":null})", "null"}
    };
    
    for (const auto& [request_json, expected_id] : test_cases) {
        std::cout << "Testing ID type: " << expected_id << std::endl;
        
        auto json_request = mcp_server->parse_json(request_json);
        
        // Extract ID the same way the server does
        std::string id;
        if (json_request.contains("id")) {
            if (json_request["id"].is_string()) {
                id = json_request["id"].get_string();
            } else if (json_request["id"].is_number()) {
                id = std::to_string(json_request["id"].get_number());
            } else if (json_request["id"].is_null()) {
                id = "null";
            }
        }
        
        std::cout << "Extracted ID: " << id << std::endl;
        EXPECT_EQ(id, expected_id);
        
        // Test the response
        std::string response = mcp_server->handle_mcp_tool_call(id, json_request);
        std::cout << "Response: " << response << std::endl;
        
        // Verify the ID is preserved in the response
        EXPECT_TRUE(response.find("\"id\":\"" + id + "\"") != std::string::npos);
    }
}
