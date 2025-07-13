#include "utils.hpp"
#include <regex>

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

bool is_youtube_url(const std::string& path) {
    std::regex youtube_regex(R"(^https?://(www\.)?(youtube\.com/watch\?v=|youtu\.be/))");
    return std::regex_search(path, youtube_regex);
}
