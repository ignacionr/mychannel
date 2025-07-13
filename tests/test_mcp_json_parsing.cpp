#include <gtest/gtest.h>
#include <glaze/glaze.hpp>
#include "../src/mcp_server.hpp"
#include "../src/http_server.hpp"
#include "../src/media_queue.hpp"

class MCPJsonParsingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock media queue and HTTP server for testing
        queue = std::make_unique<ThreadSafeMediaQueue>();
        http_server = std::make_unique<HttpServer>(*queue);
        mcp_server = std::make_unique<MCPServer>(*http_server);
    }

    std::unique_ptr<ThreadSafeMediaQueue> queue;
    std::unique_ptr<HttpServer> http_server;
    std::unique_ptr<MCPServer> mcp_server;
};

// Test basic JSON parsing with Glaze
TEST_F(MCPJsonParsingTest, BasicJsonParsing) {
    std::string json = R"({"key": "value", "number": 42, "boolean": true})";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("key"));
    ASSERT_TRUE(result["key"].is_string());
    EXPECT_EQ(result["key"].get_string(), "value");
    
    ASSERT_TRUE(result.contains("number"));
    ASSERT_TRUE(result["number"].is_number());
    EXPECT_EQ(result["number"].get_number(), 42.0);
    
    ASSERT_TRUE(result.contains("boolean"));
    ASSERT_TRUE(result["boolean"].is_boolean());
    EXPECT_EQ(result["boolean"].get<bool>(), true);
}

// Test JSON-RPC 2.0 request parsing
TEST_F(MCPJsonParsingTest, JsonRpcRequestParsing) {
    std::string json = R"({
        "jsonrpc": "2.0",
        "method": "tools/list",
        "params": {},
        "id": "test-123"
    })";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("jsonrpc"));
    EXPECT_EQ(result["jsonrpc"].get_string(), "2.0");
    
    ASSERT_TRUE(result.contains("method"));
    EXPECT_EQ(result["method"].get_string(), "tools/list");
    
    ASSERT_TRUE(result.contains("id"));
    EXPECT_EQ(result["id"].get_string(), "test-123");
    
    ASSERT_TRUE(result.contains("params"));
    ASSERT_TRUE(result["params"].is_object());
}

// Test tools/call request parsing
TEST_F(MCPJsonParsingTest, ToolsCallRequestParsing) {
    std::string json = R"({
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {
            "name": "get_streaming_queue",
            "arguments": {}
        },
        "id": "call-456"
    })";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("params"));
    ASSERT_TRUE(result["params"].is_object());
    
    auto& params = result["params"].get_object();
    ASSERT_TRUE(params.contains("name"));
    EXPECT_EQ(params.at("name").get_string(), "get_streaming_queue");
    
    ASSERT_TRUE(params.contains("arguments"));
    ASSERT_TRUE(params.at("arguments").is_object());
}

// Test tools/call with arguments
TEST_F(MCPJsonParsingTest, ToolsCallWithArguments) {
    std::string json = R"({
        "jsonrpc": "2.0",
        "method": "tools/call",
        "params": {
            "name": "add_video_to_queue",
            "arguments": {
                "source": "https://youtube.com/watch?v=test123",
                "position": "end"
            }
        },
        "id": "call-789"
    })";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("params"));
    auto& params = result["params"].get_object();
    
    ASSERT_TRUE(params.contains("arguments"));
    auto& arguments = params.at("arguments").get_object();
    
    ASSERT_TRUE(arguments.contains("source"));
    EXPECT_EQ(arguments.at("source").get_string(), "https://youtube.com/watch?v=test123");
    
    ASSERT_TRUE(arguments.contains("position"));
    EXPECT_EQ(arguments.at("position").get_string(), "end");
}

// Test JSON serialization back to string
TEST_F(MCPJsonParsingTest, JsonSerialization) {
    glz::json_t json_obj;
    json_obj["tool"] = "test_tool";
    json_obj["param1"] = "value1";
    json_obj["param2"] = 42;
    
    auto result = json_obj.dump();
    ASSERT_TRUE(result.has_value());
    
    std::string json_str = *result;
    EXPECT_TRUE(json_str.find("\"tool\":\"test_tool\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"param1\":\"value1\"") != std::string::npos);
    EXPECT_TRUE(json_str.find("\"param2\":42") != std::string::npos);
}

// Test error handling for invalid JSON
TEST_F(MCPJsonParsingTest, InvalidJsonHandling) {
    std::string invalid_json = R"({"invalid": json, missing quotes})";
    
    EXPECT_THROW(mcp_server->parse_json(invalid_json), std::runtime_error);
}

// Test empty JSON object
TEST_F(MCPJsonParsingTest, EmptyJsonObject) {
    std::string json = "{}";
    
    auto result = mcp_server->parse_json(json);
    EXPECT_TRUE(result.is_object());
    EXPECT_EQ(result.get_object().size(), 0);
}

// Test nested objects
TEST_F(MCPJsonParsingTest, NestedObjects) {
    std::string json = R"({
        "outer": {
            "inner": {
                "deep": "value"
            }
        }
    })";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("outer"));
    ASSERT_TRUE(result["outer"].is_object());
    
    auto& outer = result["outer"].get_object();
    ASSERT_TRUE(outer.contains("inner"));
    ASSERT_TRUE(outer.at("inner").is_object());
    
    auto& inner = outer.at("inner").get_object();
    ASSERT_TRUE(inner.contains("deep"));
    EXPECT_EQ(inner.at("deep").get_string(), "value");
}

// Test arrays
TEST_F(MCPJsonParsingTest, JsonArrays) {
    std::string json = R"({
        "items": ["item1", "item2", "item3"],
        "numbers": [1, 2, 3]
    })";
    
    auto result = mcp_server->parse_json(json);
    
    ASSERT_TRUE(result.contains("items"));
    ASSERT_TRUE(result["items"].is_array());
    
    auto& items = result["items"].get_array();
    EXPECT_EQ(items.size(), 3);
    EXPECT_EQ(items[0].get_string(), "item1");
    EXPECT_EQ(items[1].get_string(), "item2");
    EXPECT_EQ(items[2].get_string(), "item3");
    
    ASSERT_TRUE(result.contains("numbers"));
    ASSERT_TRUE(result["numbers"].is_array());
    
    auto& numbers = result["numbers"].get_array();
    EXPECT_EQ(numbers.size(), 3);
    EXPECT_EQ(numbers[0].get_number(), 1.0);
    EXPECT_EQ(numbers[1].get_number(), 2.0);
    EXPECT_EQ(numbers[2].get_number(), 3.0);
}

// Test extract_mcp_params function
TEST_F(MCPJsonParsingTest, ExtractMcpParams) {
    std::string json = R"({
        "tool": "test_tool",
        "params": {
            "source": "test_video.mp4",
            "position": "start",
            "priority": "high"
        }
    })";
    
    auto result = mcp_server->extract_mcp_params(json);
    
    EXPECT_EQ(result["source"], "test_video.mp4");
    EXPECT_EQ(result["position"], "start");
    EXPECT_EQ(result["priority"], "high");
}
