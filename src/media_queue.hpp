#pragma once
#include <queue>
#include <string>
#include <vector>
#include <mutex>
#include <deque>

// Thread-safe queue for media management
class ThreadSafeMediaQueue {
private:
    std::deque<std::string> queue_;  // Changed from queue to deque for front insertion
    mutable std::mutex mutex_;

public:
    void push(const std::string& item);
    void push_front(const std::string& item);  // Add high-priority item to front
    bool pop(std::string& item);
    void push_back(const std::string& item);
    size_t size() const;
    bool empty() const;
    std::vector<std::string> get_all_items() const;
    void clear();
};
