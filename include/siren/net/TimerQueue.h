#pragma once
#include "siren/net/Callbacks.h"
#include "siren/net/Channel.h"
#include "siren/net/EventLoop.h"
#include "siren/net/TimerId.h"

#include <set>
#include <vector>


namespace siren {
namespace net {

// class Channel;
class EventLoop;
class Timer;
class TimerId;
class TimerCmp;

class TimerQueue {
   public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb, Timestamp when, int64_t interval);
    void cancel(TimerId timerId);

   private:
    typedef std::set<Timer*, TimerCmp> TimerList;
    void addTimerInLoop(Timer* timer);
    void handleRead();

    // move out all expired timers
    std::vector<Timer*> getExpired(Timestamp now);
    void reset(const std::vector<Timer*>& expired, Timestamp now);

    bool insert(Timer* timer);

    bool callingExpiredTimers_;
    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    // Timer list sorted by expiration
    TimerList timers_;
};
}  // namespace net

}  // namespace siren


