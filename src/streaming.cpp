#include "streaming.hpp"
#include "streaming_config.hpp"
#include "utils.hpp"
#include <iostream>
#include <future>
#include <sstream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <sys/select.h>
#include <cstdlib>

// Global stream process manager
std::shared_ptr<StreamProcess> g_stream_process = std::make_shared<StreamProcess>();

StreamProcess::StreamProcess() : current_pid_(0), should_terminate_(false) {}

void StreamProcess::set_current_pid(pid_t pid) {
    current_pid_.store(pid);
}

void StreamProcess::request_termination() {
    should_terminate_.store(true);
}

bool StreamProcess::should_terminate() const {
    return should_terminate_.load();
}

void StreamProcess::kill_current_process() {
    pid_t pid = current_pid_.load();
    if (pid > 0) {
        std::cout << "🛑 Terminating current stream process (PID: " << pid << ")" << std::endl;
        
        // First try to terminate gracefully
        if (kill(pid, SIGTERM) == 0) {
            std::cout << "Sent SIGTERM to process " << pid << std::endl;
            
            // Give process time to terminate gracefully
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            
            // Check if process is still running
            if (kill(pid, 0) == 0) {
                std::cout << "🔥 Process still running, force killing with SIGKILL..." << std::endl;
                kill(pid, SIGKILL);
                
                // Also try to kill the process group in case there are child processes
                kill(-pid, SIGKILL);
                
                // Wait a bit more
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            } else {
                std::cout << "✅ Process terminated gracefully" << std::endl;
            }
        } else {
            std::cout << "⚠️ Failed to send signal to process " << pid << " (may have already terminated)" << std::endl;
        }
    } else {
        std::cout << "⚠️ No current process PID available, trying pattern-based kill..." << std::endl;
    }
    
    // As a fallback, always try to kill any ffmpeg processes that might be streaming
    std::cout << "🔪 Attempting to kill all ffmpeg streaming processes..." << std::endl;
    int result = system("pkill -f 'ffmpeg.*rtmp'");
    if (result == 0) {
        std::cout << "✅ Successfully killed ffmpeg streaming processes" << std::endl;
    } else {
        std::cout << "⚠️ No ffmpeg streaming processes found or kill failed" << std::endl;
    }
}

void StreamProcess::reset() {
    current_pid_.store(0);
    should_terminate_.store(false);
}

std::future<void> push_to_youtube_async(const std::string& video_path, const std::string& rtmp_url, const std::string& stream_key) {
    return std::async(std::launch::async, [video_path, rtmp_url, stream_key]() {
        if (rtmp_url.empty() || stream_key.empty()) {
            std::cerr << "Error: RTMP URL or Stream Key is empty." << std::endl;
            std::cout << "Skipping YouTube push for " << video_path << std::endl;
            return;
        }

        std::cout << "Pushing " << video_path << " to YouTube Live Stream..." << std::endl;
        std::cout << "🎬 Quality Settings: " << StreamingConfig::MAX_HEIGHT << "p @ " 
                  << StreamingConfig::VIDEO_BITRATE << "k video, " 
                  << StreamingConfig::AUDIO_BITRATE << "k audio" << std::endl;

        std::string ffmpeg_command;
        
        if (is_youtube_url(video_path)) {
            // For YouTube URLs, use yt-dlp to pipe the stream directly to ffmpeg
            std::cout << "Detected YouTube URL, using yt-dlp to stream..." << std::endl;
            
            std::stringstream cmd;
            cmd << "yt-dlp -f 'best[height<=" << StreamingConfig::MAX_HEIGHT << "]' -o - " << video_path
                << " | /nix/store/dfc4gg05vh5wini7z0wvia3x0slszqxi-ffmpeg-7.1.1-bin/bin/ffmpeg -re -i pipe:0"
                << " -c:v libx264 -preset " << StreamingConfig::VIDEO_PRESET 
                << " -crf " << StreamingConfig::CRF_VALUE
                << " -maxrate " << StreamingConfig::VIDEO_BITRATE << "k"
                << " -bufsize " << StreamingConfig::BUFFER_SIZE << "k"
                << " -pix_fmt " << StreamingConfig::PIXEL_FORMAT
                << " -g " << StreamingConfig::GOP_SIZE
                << " -c:a aac -b:a " << StreamingConfig::AUDIO_BITRATE << "k"
                << " -ar " << StreamingConfig::AUDIO_SAMPLE_RATE
                << " -f flv " << rtmp_url << "/" << stream_key;
            ffmpeg_command = cmd.str();
        } else {
            // For local files, use the original ffmpeg command
            std::stringstream cmd;
            cmd << "/nix/store/dfc4gg05vh5wini7z0wvia3x0slszqxi-ffmpeg-7.1.1-bin/bin/ffmpeg -re -i " << video_path
                << " -c:v libx264 -preset " << StreamingConfig::VIDEO_PRESET
                << " -crf " << StreamingConfig::CRF_VALUE
                << " -maxrate " << StreamingConfig::VIDEO_BITRATE << "k"
                << " -bufsize " << StreamingConfig::BUFFER_SIZE << "k"
                << " -pix_fmt " << StreamingConfig::PIXEL_FORMAT
                << " -g " << StreamingConfig::GOP_SIZE
                << " -c:a aac -b:a " << StreamingConfig::AUDIO_BITRATE << "k"
                << " -ar " << StreamingConfig::AUDIO_SAMPLE_RATE
                << " -f flv " << rtmp_url << "/" << stream_key;
            ffmpeg_command = cmd.str();
        }

        try {
            std::cout << "🎬 Starting ffmpeg command: " << ffmpeg_command << std::endl;
            
            // Use popen but with process group management
            FILE* pipe = popen(ffmpeg_command.c_str(), "r");
            if (!pipe) {
                throw std::runtime_error("Failed to start ffmpeg process");
            }
            
            // Get the process ID of the popen command
            // We'll use a different approach - find the ffmpeg process by command line
            std::string pid_command = "pgrep -f 'ffmpeg.*" + rtmp_url + "'";
            FILE* pid_pipe = popen(pid_command.c_str(), "r");
            
            pid_t ffmpeg_pid = 0;
            if (pid_pipe) {
                char pid_buffer[32];
                if (fgets(pid_buffer, sizeof(pid_buffer), pid_pipe)) {
                    ffmpeg_pid = std::stoi(std::string(pid_buffer));
                    g_stream_process->set_current_pid(ffmpeg_pid);
                    std::cout << "🎬 Found ffmpeg process with PID: " << ffmpeg_pid << std::endl;
                }
                pclose(pid_pipe);
            }
            
            // If we couldn't find the PID, set a placeholder (the shell PID)
            if (ffmpeg_pid == 0) {
                std::cout << "⚠️ Could not determine exact ffmpeg PID, using shell process for termination" << std::endl;
                // We'll use a different strategy - track by command pattern
            }
            
            char buffer[256];
            std::string result = "";
            bool process_terminated = false;
            
            // Read output while checking for termination requests
            while (!process_terminated) {
                if (g_stream_process->should_terminate()) {
                    std::cout << "🛑 Stream termination requested, stopping..." << std::endl;
                    
                    // Kill all ffmpeg processes related to our stream
                    std::string kill_command = "pkill -f 'ffmpeg.*" + rtmp_url + "'";
                    std::cout << "🔪 Executing kill command: " << kill_command << std::endl;
                    int kill_result = system(kill_command.c_str());
                    std::cout << "Kill command result: " << kill_result << std::endl;
                    
                    // Close the pipe
                    pclose(pipe);
                    return;
                }
                
                // Try to read from pipe with timeout
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(fileno(pipe), &read_fds);
                
                struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000; // 100ms
                
                int select_result = select(fileno(pipe) + 1, &read_fds, nullptr, nullptr, &timeout);
                
                if (select_result > 0 && FD_ISSET(fileno(pipe), &read_fds)) {
                    if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                        result += buffer;
                        std::cout << "ffmpeg: " << buffer; // Show output in real-time
                    } else {
                        // End of stream
                        process_terminated = true;
                    }
                } else if (select_result == 0) {
                    // Timeout - continue checking for termination
                    continue;
                } else {
                    // Error or end of stream
                    process_terminated = true;
                }
            }
            
            int status = pclose(pipe);
            if (status == 0) {
                std::cout << "Successfully pushed " << video_path << " to YouTube Live Stream." << std::endl;
            } else if (!g_stream_process->should_terminate()) {
                std::cout << "ffmpeg process ended with status: " << status << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error pushing to YouTube: " << e.what() << std::endl;
        }
        
        g_stream_process->reset();
    });
}
