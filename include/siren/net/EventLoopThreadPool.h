#pragma once

#include "siren/base/Types.h"
#include "siren/base/noncopyable.h"

#include <functional>
#include <memory>
#include <vector>


namespace siren::net {
    class EventLoop;
    class EventLoopThread;

    class EventLoopThreadPool : public noncopyable {
    public:
        using ThreadInitCallback = std::function<void(EventLoop*)>;
        EventLoopThreadPool(EventLoop* baseLoop, string  nameArg);
        ~EventLoopThreadPool();
        void setThreadNum(int numThreads) { numThreads_ = numThreads; }
        void start(const ThreadInitCallback& cb = ThreadInitCallback());

        // valid after calling start()
        /// round-robin
        EventLoop* getNextLoop();

        /// with the same hash code, it will always return the same EventLoop
        EventLoop* getLoopForHash(size_t hashCode);

        std::vector<EventLoop*> getAllLoops();

        [[nodiscard]] bool started() const
        {
            return started_;
        }

        [[nodiscard]] const string& name() const
        {
            return name_;
        }

    private:
        EventLoop* baseLoop_;
        string name_;
        bool started_;
        int numThreads_;
        int next_;
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
    };
} // namespace siren::net


