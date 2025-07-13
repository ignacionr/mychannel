#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <future>
#include "media_queue.hpp"
#include "media_info.hpp"
#include "streaming.hpp"
#include "http_server.hpp"
#include "mcp_server.hpp"
#include "utils.hpp"

int main() {
    const char* rtmp_url_env = std::getenv("YOUTUBE_RTMP_URL");
    const char* stream_key_env = std::getenv("YOUTUBE_STREAM_KEY");

    if (!rtmp_url_env || !stream_key_env) {
        std::cerr << "Error: YOUTUBE_RTMP_URL or YOUTUBE_STREAM_KEY environment variables are not set." << std::endl;
        return 1;
    }

    std::string rtmp_url = rtmp_url_env;
    std::string stream_key = stream_key_env;

    // Initialize media queue with default items
    ThreadSafeMediaQueue media_queue;
    // media_queue.push("https://www.youtube.com/watch?v=gCNeDWCI0vo");

    // Start HTTP server with MCP support
    HttpServer http_server(media_queue);
    MCPServer mcp_server(http_server);
    auto server_future = http_server.start_async();

    std::future<void> current_push_future;

    // Main streaming loop
    for (;;) {
        std::string current_video_path;
        bool is_fallback = false;
        
        if (!media_queue.pop(current_video_path)) {
            // Queue is empty, use fallback video
            current_video_path = "videos/News_Intro.mp4";
            is_fallback = true;
            std::cout << "Queue is empty, playing fallback video: " << current_video_path << std::endl;
        } else {
            // Add item back to end of queue for continuous loop
            media_queue.push_back(current_video_path);
        }

        // Get media duration
        double duration;
        if (is_youtube_url(current_video_path)) {
            std::cout << "Getting duration for YouTube video: " << current_video_path << std::endl;
            duration = get_youtube_duration(current_video_path);
        } else {
            duration = get_media_duration(current_video_path);
        }
        
        std::cout << "Media duration for " << current_video_path << ": " << duration << " seconds" << std::endl;
        
        if (is_fallback) {
            std::cout << "📺 [FALLBACK] No queue items - streaming default content" << std::endl;
        } else {
            std::cout << "🎬 [QUEUE] Streaming queued content" << std::endl;
        }

        // Start async streaming
        g_stream_process->reset(); // Reset termination flag
        current_push_future = push_to_youtube_async(current_video_path, rtmp_url, stream_key);
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Simulate playback timing with interruption checking
        for (int j = 0; j < static_cast<int>(duration); ++j) {
            // Check if stream should be interrupted
            if (g_stream_process->should_terminate()) {
                std::cout << "🔄 Stream interrupted for high-priority content" << std::endl;
                break;
            }
            
            std::cout << "Playing " << current_video_path << " - " << (j + 1) << " of " << static_cast<int>(duration) << " seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Check for interruption before handling fractional duration
        if (!g_stream_process->should_terminate()) {
            // Handle fractional duration
            if (duration - static_cast<int>(duration) > 0.0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>((duration - static_cast<int>(duration)) * 1000)));
            }
        }
        
        std::cout << "Finished playing " << current_video_path << std::endl;
        
        // Wait for the streaming future to complete before continuing
        if (current_push_future.valid()) {
            current_push_future.wait();
        }
        
        std::cout << "----------------------------------------" << std::endl;
    }

    return 0;
}
