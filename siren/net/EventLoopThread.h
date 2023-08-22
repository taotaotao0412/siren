#pragma once
#include "siren/base/noncopyable.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace siren::net {
    class EventLoop;
    class EventLoopThread : public noncopyable {
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        explicit EventLoopThread(ThreadInitCallback  cb = ThreadInitCallback());
        ~EventLoopThread();
        EventLoop* startLoop();
        
    private:
        void threadFunc();

        EventLoop* loop_;
        bool exiting_;
        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable cond_;
        ThreadInitCallback callback_;
    };
} // namespace siren::net


