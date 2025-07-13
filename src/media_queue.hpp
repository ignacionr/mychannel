#pragma once
#include <queue>
#include <string>
#include <vector>
#include <mutex>

// Thread-safe queue for media management
class ThreadSafeMediaQueue {
private:
    std::queue<std::string> queue_;
    mutable std::mutex mutex_;

public:
    void push(const std::string& item);
    bool pop(std::string& item);
    void push_back(const std::string& item);
    size_t size() const;
    bool empty() const;
    std::vector<std::string> get_all_items() const;
    void clear();
};
