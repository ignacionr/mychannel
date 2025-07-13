#pragma once
#include <string>
#include <future>

// Asynchronous streaming function
std::future<void> push_to_youtube_async(
    const std::string& video_path, 
    const std::string& rtmp_url, 
    const std::string& stream_key
);
