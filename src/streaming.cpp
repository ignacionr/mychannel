#include "streaming.hpp"
#include "streaming_config.hpp"
#include "utils.hpp"
#include <iostream>
#include <future>
#include <sstream>

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
            std::string output = exec(ffmpeg_command.c_str());
            std::cout << "ffmpeg output: " << output << std::endl;
            std::cout << "Successfully pushed " << video_path << " to YouTube Live Stream." << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error pushing to YouTube: " << e.what() << std::endl;
        }
    });
}
