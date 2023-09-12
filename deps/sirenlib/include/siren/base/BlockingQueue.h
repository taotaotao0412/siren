#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

#include "siren/base/noncopyable.h"

namespace siren {
template <typename T>
class BlockingQueue : noncopyable {
   public:
    using queue_type = std::deque<T>;

    BlockingQueue() : mutex_(), notEmpty_(), queue_() {}

    void put(const T &x) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(x);
        notEmpty_.notify_one();
    }

    void put(T &&x) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push_back(std::move(x));
        notEmpty_.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (queue_.empty()) {
            notEmpty_.wait(lock);
        }
        T front(std::move(queue_.front()));
        queue_.pop_front();
        return front;
    }

    /**
     * @brief 将队列所有元素取出
     * @note 线程安全
     *
     * @return queue: BlockingQueue
     */
    queue_type drain() {
        std::deque<T> queue;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            queue = std::move(queue_);
        }
        return queue;
    }

    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }

   private:
    mutable std::mutex mutex_;
    std::condition_variable notEmpty_;
    queue_type queue_;
};
}  // namespace siren