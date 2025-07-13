#include "media_queue.hpp"

void ThreadSafeMediaQueue::push(const std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(item);
}

void ThreadSafeMediaQueue::push_front(const std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_front(item);
}

bool ThreadSafeMediaQueue::pop(std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    item = queue_.front();
    queue_.pop_front();
    return true;
}

void ThreadSafeMediaQueue::push_back(const std::string& item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(item);
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
    for (const auto& item : queue_) {
        items.push_back(item);
    }
    return items;
}

void ThreadSafeMediaQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.clear();
}
