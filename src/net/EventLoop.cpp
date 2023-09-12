#include "siren/net/EventLoop.h"

#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>

#include "siren/net/SocketsOps.h"
#include "fmt/std.h"

using namespace siren::net;
namespace {
int createEventfd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        LOG_ERROR("Failed in eventfd");
        abort();
    }
    return evtfd;
}
}  // namespace

std::string threadId2string(std::thread::id tid) {
    return fmt::format("{}", tid);
    auto myid = tid;
    std::stringstream ss;
    ss << myid;
    return ss.str();
}

siren::net::EventLoop::EventLoop()
    : looping_(false),
      threadId_(std::this_thread::get_id()),
      eventHandling_(false),
      callingPendingFunctors_(false),
      quit_(false),
      poller_(Poller::newDefaultPoller(this)),
      timerQueue_(new TimerQueue(this)),
      kPollTimeMs(1000 * 10),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)) {
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

siren::net::EventLoop::~EventLoop() {
    // t_loopInThisThread = nullptr;
    LOG_DEBUG("EventLoop {} of thread {} destructs in thread ", fmt::ptr(this),
              threadId_, std::this_thread::get_id());
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    // t_loopInThisThread = NULL;
}

bool siren::net::EventLoop::isInLoopThread() const {
    return threadId_ == std::this_thread::get_id();
}

void siren::net::EventLoop::assertInLoopThread() {
    if (!isInLoopThread()) abortNotInLoopThread();
}
void siren::net::EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    assertInLoopThread();
    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        // printactivatechannels
        eventHandling_ = true;
        for (auto channel : activeChannels_) {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }

        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        doPendingFunctors();  // 处理runInLoop()部分函数
    }

    looping_ = false;
}

bool siren::net::EventLoop::hasChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}

void siren::net::EventLoop::updateChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);
}
void siren::net::EventLoop::removeChannel(Channel* channel) {
    assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
        std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}
void siren::net::EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::wakeup() writes {} bytes instead of 8", n);
    }
}

void siren::net::EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("EventLoop::handleRead() reads {} bytes instead of 8", n);
    }
}

void siren::net::EventLoop::abortNotInLoopThread() {
    LOG_ERROR(
        "EventLoop::abortNotInLoopThread - EventLoop {} was created in "
        "threadId_ = {}, current thread id = {}",
        1, threadId2string(threadId_),
        threadId2string(std::this_thread::get_id()));
}

void siren::net::EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors) {
        if(functor)
            functor();
    }
    callingPendingFunctors_ = false;
}

void siren::net::EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void siren::net::EventLoop::queueInLoop(Functor cb) {
    if(!cb){
        LOG_ERROR("queueinloop has empty function!");
    }
    {
        std::unique_lock<std::mutex>(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void siren::net::EventLoop::quit() {
    quit_ = true;

    if (!isInLoopThread()) {
        wakeup();
    }
}

/**
 * @brief 立刻添加EventLoop事件到TimerQueue中
 *
 * @param time 是std::chrono::system_clock::time_point
 * 对象，表示事件发生的时间点
 * @param cb callback
 * @return TimerId, Timer的句柄，可以认为是Timer的ID号
 * @note 从其他线程调用是线程安全的
 */
siren::net::TimerId siren::net::EventLoop::runAt(Timestamp time,
                                                 TimerCallback cb) {
    return timerQueue_->addTimer(std::move(cb), time, 0);
}

/**
 * @brief 立刻添加EventLoop事件到TimerQueue中，并延迟发生
 *
 * @param delay 一个 double 值，延迟发生都秒数
 * @param cb callback
 * @return TimerId, Timer的句柄，可以认为是Timer的ID号
 * @note 从其他线程调用是线程安全的
 */
TimerId siren::net::EventLoop::runAfter(double delay, TimerCallback cb) {
    auto now = std::chrono::system_clock::now();
    auto time =
        now + std::chrono::milliseconds(static_cast<int64_t>(delay * 1000));
    return runAt(time, std::move(cb));
}

/**
 * @brief 添加EventLoop循环事件
 *
 * @param interval 事件发生间隔，单位：秒
 * @param cb callback
 * @return TimerId, Timer的句柄，可以认为是Timer的ID号
 * @note 从其他线程调用是线程安全的
 */
TimerId siren::net::EventLoop::runEvery(double interval, TimerCallback cb) {
    auto now = std::chrono::system_clock::now();
    auto time =
        now + std::chrono::milliseconds(static_cast<int64_t>(interval * 1000));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}
