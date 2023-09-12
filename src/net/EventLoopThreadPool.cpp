#include "siren/net/EventLoopThreadPool.h"

#include <utility>
#include "siren/net/EventLoop.h"
#include "siren/net/EventLoopThread.h"

using namespace siren::net;

siren::net::EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, string nameArg)
        : baseLoop_(baseLoop), name_(std::move(nameArg)), started_(false), numThreads_(0), next_(0) {
}

siren::net::EventLoopThreadPool::~EventLoopThreadPool()
= default;

void siren::net::EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    assert(!started_);
    baseLoop_->assertInLoopThread();
    started_ = true;
    for (int i = 0; i < numThreads_; i++) {
        auto t = new EventLoopThread(cb);
        threads_.emplace_back(t);
        loops_.push_back(t->startLoop());
    }
    if (numThreads_ == 0 && cb) {
        cb(baseLoop_);
    }
}

EventLoop *siren::net::EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;

    if (!loops_.empty()) {
        loop = loops_[next_];
        next_++;
        if (static_cast<size_t>(next_) >= loops_.size())
            next_ = 0;
    }
    return loop;
}

EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode) {
    baseLoop_->assertInLoopThread();
    EventLoop *loop = baseLoop_;

    if (!loops_.empty()) {
        loop = loops_[hashCode % loops_.size()];
    }
    return loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    baseLoop_->assertInLoopThread();
    assert(started_);
    if (loops_.empty()) {
        return std::vector<EventLoop *>{baseLoop_};
    } else {
        return loops_;
    }
}
