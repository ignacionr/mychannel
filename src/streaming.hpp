#pragma once
#include <string>
#include <future>
#include <atomic>
#include <memory>

// Process management for controlling ffmpeg streams
class StreamProcess {
private:
    std::atomic<pid_t> current_pid_;
    std::atomic<bool> should_terminate_;

public:
    StreamProcess();
    void set_current_pid(pid_t pid);
    void request_termination();
    bool should_terminate() const;
    void kill_current_process();
    void reset();
};

// Global stream process manager
extern std::shared_ptr<StreamProcess> g_stream_process;

// Asynchronous streaming function
std::future<void> push_to_youtube_async(
    const std::string& video_path, 
    const std::string& rtmp_url, 
    const std::string& stream_key
);
