#pragma once
#include <string>

// Function to get media duration using ffprobe
double get_media_duration(const std::string& video_path);

// Function to get YouTube video duration using yt-dlp
double get_youtube_duration(const std::string& youtube_url);
