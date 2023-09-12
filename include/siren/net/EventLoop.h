#pragma once
#include "siren/base/Logger.h"
#include "siren/base/noncopyable.h"
#include "siren/net/Poller.h"
#include "siren/net/TimerId.h"
#include "siren/net/TimerQueue.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>



namespace siren::net {

    class Channel;
    class Poller;
    class TimerQueue;

    using ChannelList = std::vector<Channel*>;
    typedef std::function<void()> Functor;

    class EventLoop : noncopyable {
    public:
        EventLoop();
        ~EventLoop();
        bool isInLoopThread() const;
        void assertInLoopThread();
        void loop();
        bool hasChannel(Channel* channel);

        void updateChannel(Channel*);
        void removeChannel(Channel* channel);
        void wakeup();
        void runInLoop(Functor cb);
        void queueInLoop(Functor cb);
        void quit();

        TimerId runAt(Timestamp time, TimerCallback cb);

        /**
         * @brief Runs callback after description delay seconds.
         *
         * @param delay delay seconds.
         * @param cb callback function
         * @return TimerId
         */
        TimerId runAfter(double delay, TimerCallback cb);

        /**
         * @brief 添加EventLoop循环事件
         *
         * @param interval 事件发生间隔，单位：秒
         * @param cb callback
         * @return TimerId, Timer的句柄，可以认为是Timer的ID号
         * @note 从其他线程调用是线程安全的
         */
        TimerId runEvery(double interval, TimerCallback cb);

    private:
        const int kPollTimeMs; // poll 超时时间
        const std::thread::id threadId_; // handle eventloop的线程
        void handleRead(); // wakeup
        void abortNotInLoopThread();
        void doPendingFunctors();
        bool looping_;
        bool callingPendingFunctors_; /* atomic */
        std::atomic<bool> quit_;
        std::atomic<bool> eventHandling_;
        std::unique_ptr<Poller> poller_;
        ChannelList activeChannels_;
        Channel* currentActiveChannel_{};
        Timestamp pollReturnTime_;
        std::unique_ptr<TimerQueue> timerQueue_;

          int wakeupFd_;

        std::unique_ptr<Channel> wakeupChannel_;

        mutable std::mutex mutex_;
        std::vector<Functor> pendingFunctors_;
        
    };
} // namespace siren::net

