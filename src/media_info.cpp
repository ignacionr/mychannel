#include "media_info.hpp"
#include "utils.hpp"
#include <iostream>
#include <sstream>
#include <vector>

double get_media_duration(const std::string& video_path) {
    std::string command = "/nix/store/dfc4gg05vh5wini7z0wvia3x0slszqxi-ffmpeg-7.1.1-bin/bin/ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 " + video_path;
    std::string duration_str = exec(command.c_str());
    try {
        return std::stod(duration_str);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing duration for " << video_path << ": " << e.what() << std::endl;
        return 0.0;
    }
}

double get_youtube_duration(const std::string& youtube_url) {
    std::string command = "yt-dlp --get-duration --no-warnings " + youtube_url;
    std::string duration_str = exec(command.c_str());
    
    // Remove any trailing whitespace/newlines
    duration_str.erase(duration_str.find_last_not_of(" \t\n\r") + 1);
    
    try {
        // Parse duration format (e.g., "3:45" or "1:23:45")
        std::vector<int> parts;
        std::stringstream ss(duration_str);
        std::string part;
        
        while (std::getline(ss, part, ':')) {
            parts.push_back(std::stoi(part));
        }
        
        double total_seconds = 0;
        if (parts.size() == 2) { // MM:SS
            total_seconds = parts[0] * 60 + parts[1];
        } else if (parts.size() == 3) { // HH:MM:SS
            total_seconds = parts[0] * 3600 + parts[1] * 60 + parts[2];
        } else if (parts.size() == 1) { // Just seconds
            total_seconds = parts[0];
        }
        
        return total_seconds;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing YouTube duration for " << youtube_url << ": " << e.what() << std::endl;
        return 0.0;
    }
}
