#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <queue>
#include <cstdlib> // For std::getenv

// Function to execute a shell command and return its output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Function to get media duration using ffprobe
double get_media_duration(const std::string& video_path) {
    std::string command = "/nix/store/dfc4gg05vh5wini7z0wvia3x0slszqxi-ffmpeg-7.1.1-bin/bin/ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + video_path;
    std::string duration_str = exec(command.c_str());
    try {
        return std::stod(duration_str);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing duration for " << video_path << ": " << e.what() << std::endl;
        return 0.0; // Return 0 or throw an error
    }
}

void push_to_youtube(const std::string& video_path, const std::string& rtmp_url, const std::string& stream_key) {
    if (rtmp_url.empty() || stream_key.empty()) {
        std::cerr << "Error: RTMP URL or Stream Key is empty." << std::endl;
        std::cout << "Skipping YouTube push for " << video_path << std::endl;
        return;
    }

    std::cout << "Pushing " << video_path << " to YouTube Live Stream..." << std::endl;

    // Construct the ffmpeg command for live streaming
    // This is a basic example, you might need to adjust parameters based on your stream requirements
    std::string ffmpeg_command = "/nix/store/dfc4gg05vh5wini7z0wvia3x0slszqxi-ffmpeg-7.1.1-bin/bin/ffmpeg -re -i " + video_path +
                                 " -c:v libx264 -preset veryfast -maxrate 3000k -bufsize 6000k -pix_fmt yuv420p -g 50 -c:a aac -b:a 128k -ar 44100 -f flv " +
                                 rtmp_url + "/" + stream_key;

    try {
        std::string output = exec(ffmpeg_command.c_str());
        std::cout << "ffmpeg output: " << output << std::endl;
        std::cout << "Successfully pushed " << video_path << " to YouTube Live Stream." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error pushing to YouTube: " << e.what() << std::endl;
    }
}

int main() {
    const char* rtmp_url_env = std::getenv("YOUTUBE_RTMP_URL");
    const char* stream_key_env = std::getenv("YOUTUBE_STREAM_KEY");

    if (!rtmp_url_env || !stream_key_env) {
        std::cerr << "Error: YOUTUBE_RTMP_URL or YOUTUBE_STREAM_KEY environment variables are not set. Please set them in your shell environment (e.g., .zprofile) and source it." << std::endl;
        return 1; // Exit with an error code
    }

    std::string rtmp_url = rtmp_url_env;
    std::string stream_key = stream_key_env;

    std::queue<std::string> media_queue;
    media_queue.push("videos/video1_5s.webm");
    media_queue.push("videos/video2_5s.webm");

    for (int i = 0; i < 2; ++i) { // Infinite loop
        std::string current_video_path = media_queue.front();
        media_queue.pop(); // Remove from front
        media_queue.push(current_video_path); // Add to back for continuous loop

        double duration = get_media_duration(current_video_path);
        std::cout << "Media duration for " << current_video_path << ": " << duration << " seconds" << std::endl;

        // Simulate the "pushing" action
        push_to_youtube(current_video_path, rtmp_url, stream_key);
        std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate upload time

        // Simulate playing and report every second
        for (int j = 0; j < static_cast<int>(duration); ++j) {
            std::cout << "Playing " << current_video_path << " - " << (j + 1) << " of " << static_cast<int>(duration) << " seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        // Handle any remaining fractional part of the duration
        if (duration - static_cast<int>(duration) > 0.0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>((duration - static_cast<int>(duration)) * 1000)));
        }
        std::cout << "Finished playing " << current_video_path << std::endl;
        std::cout << "----------------------------------------" << std::endl; // Separator for clarity
    }

    return 0;
}
