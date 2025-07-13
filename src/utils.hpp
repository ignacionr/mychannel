#pragma once
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <cstdio>

// Function to execute a shell command and return its output
std::string exec(const char* cmd);

// Function to check if a string is a YouTube URL
bool is_youtube_url(const std::string& path);
