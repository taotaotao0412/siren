#pragma once

#include "siren/base/noncopyable.h"
#include "siren/net/Callbacks.h"

#include <atomic>
#include <chrono>
#include <functional>


namespace siren {
namespace net {
    using Timestamp = std::chrono::system_clock::time_point;

    class Timer : noncopyable {
    public:
        Timer(TimerCallback cb, std::chrono::system_clock::time_point when,
            double interval)
            : callback_(std::move(cb))
            , expiration_(when)
            , interval_(interval)
            , repeat_(interval > 0.0)
            , sequence_(Timer::s_numCreated_.fetch_add(1))
        {
        }
        void restart(std::chrono::system_clock::time_point);
        std::chrono::system_clock::time_point expiration() const
        {
            return expiration_;
        }
        void run() const { callback_(); }
        bool repeat() const { return repeat_; }
        int64_t sequence() const { return sequence_; }
        static int64_t numCreated() { return s_numCreated_.load(); }

    private:
        const TimerCallback callback_;
        std::chrono::system_clock::time_point expiration_;
        const double interval_; // second
        const bool repeat_;
        const int64_t sequence_;
        static std::atomic<long long> s_numCreated_;
    };

    struct TimerCmp {
        bool operator()(const Timer* t1, const Timer* t2) const
        {
            return t1->expiration() < t2->expiration();
        }
    };
} // namespace net
} // namespace siren
