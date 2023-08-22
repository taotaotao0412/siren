#include "siren/net/TimerQueue.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace siren {
namespace net {
namespace detail {

int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        LOG_ERROR("Failed in timerfd_create");
    }
    return timerfd;
}

void readTimerfd(int timerfd, Timestamp now) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    LOG_TRACE("TimerQueue::handleRead() n = {}", howmany);
    if (n != sizeof howmany) {
        LOG_ERROR("TimerQueue::handleRead() reads {} bytes instead of 8", n);
    }
}

struct timespec howMuchTimeFromNow(Timestamp when) {
    auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(
                            when - std::chrono::system_clock::now())
                            .count();

    const int64_t minDelayMicroseconds = 100;
    // microseconds = std::max(static_cast<int64_t>(0), microseconds);
    microseconds =
        std::max(microseconds, minDelayMicroseconds);  // 延迟时间最少为0.1ms
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / 1000 / 1000);
    ts.tv_nsec = static_cast<long>((microseconds % (1000 * 1000)) * 1000);
    return ts;
}

void resetTimerfd(int timerfd, Timestamp expiration) {
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof newValue);
    memset(&oldValue, 0, sizeof oldValue);
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        LOG_ERROR("timerfd_settime(...)");
    }
}

}  // namespace detail
}  // namespace net
}  // namespace siren

using namespace siren;
using namespace siren::net;
using namespace siren::net::detail;

siren::net::TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannel_(loop, timerfd_),
      timers_() {
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannel_.enableReading();
}

siren::net::TimerQueue::~TimerQueue() {
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    
    for(const auto &k: timers_){
        delete k;
    }
}

TimerId siren::net::TimerQueue::addTimer(TimerCallback cb, Timestamp when,
                                         int64_t interval) {
    auto timer = new Timer(std::move(cb), when, interval);
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer));

    return TimerId(timer, timer->sequence());
}

void siren::net::TimerQueue::cancel(TimerId timerId) {}

void siren::net::TimerQueue::addTimerInLoop(Timer* timer) {
    loop_->assertInLoopThread();
    bool earliestChanged = insert(timer);

    if (earliestChanged) resetTimerfd(timerfd_, timer->expiration());
}

void siren::net::TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    Timestamp now = std::chrono::system_clock::now();
    readTimerfd(timerfd_, now);

    std::vector<Timer*> expired = getExpired(now);

    callingExpiredTimers_ = true;
    // safe to callback outside critical section
    for (auto iter : expired) {
        iter->run();
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}
void siren::net::TimerQueue::reset(const std::vector<Timer*>& expired,
                                   Timestamp now) {
    Timestamp nextExpire;
    for (auto iter : expired) {
        if (iter->repeat()) {
            iter->restart(now);
            insert(iter);
        } else {
            // FIXME move to a free list
            delete iter;  // FIXME: no delete please$
        }
    }

    if (!timers_.empty()) {
        nextExpire = (*timers_.begin())->expiration();
        resetTimerfd(timerfd_, nextExpire);
    }

    // resetTimerfd(timerfd_, nextExpire);  // check timestamp fd??
}

std::vector<Timer*> siren::net::TimerQueue::getExpired(Timestamp now) {
    std::vector<Timer*> expired;
    if (timers_.empty()) return expired;

    std::shared_ptr<Timer> sentryPtr = std::make_shared<Timer>([]() {}, now, 0);
    auto end = timers_.upper_bound(sentryPtr.get());

    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);
    return expired;
}

bool siren::net::TimerQueue::insert(Timer* timer) {
    loop_->assertInLoopThread();

    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    auto iter = timers_.begin();
    if (iter == timers_.end() || when < (*iter)->expiration())
        earliestChanged = true;
    auto result = timers_.insert(timer);

    return earliestChanged;
}
