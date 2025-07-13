#include "media_queue.hpp"

void ThreadSafeMediaQueue::push(const std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(item);
}

bool ThreadSafeMediaQueue::pop(std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    item = queue_.front();
    queue_.pop();
    return true;
}

void ThreadSafeMediaQueue::push_back(const std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(item);
}

size_t ThreadSafeMediaQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool ThreadSafeMediaQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

std::vector<std::string> ThreadSafeMediaQueue::get_all_items() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> items;
    std::queue<std::string> temp_queue = queue_;
    while (!temp_queue.empty()) {
        items.push_back(temp_queue.front());
        temp_queue.pop();
    }
    return items;
}

void ThreadSafeMediaQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}
