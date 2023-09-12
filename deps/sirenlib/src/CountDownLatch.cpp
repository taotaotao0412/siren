#include "siren/base/CountDownLatch.h"

/**
 * @brief Construct a new siren::Count Down Latch::Count Down Latch object
 *
 * @param count 计数器初始值
 */
siren::CountDownLatch::CountDownLatch(int count)
    : count_(count), mutex_(), condition_() {}

void siren::CountDownLatch::wait() {
    std::unique_lock<std::mutex> lock(mutex_);

    while (count_ > 0) {
        condition_.wait(lock);
    }
}

/**
 * @brief 将计数器减少1
 *
 */
void siren::CountDownLatch::countDown() {
    std::unique_lock<std::mutex> lock(mutex_);
    count_--;
    if (count_ == 0) condition_.notify_all();
}

int siren::CountDownLatch::getCount() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return count_;
}