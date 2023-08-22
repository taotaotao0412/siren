#include "siren/net/EventLoopThread.h"
#include "siren/net/EventLoop.h"
#include <assert.h>

#include <utility>

using namespace siren::net;

siren::net::EventLoopThread::EventLoopThread(ThreadInitCallback  cb)
    : loop_(nullptr)
    , exiting_(false)
    , mutex_()
    , callback_(std::move(cb))
{
}
siren::net::EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr) {
        loop_->quit();
        if (thread_.joinable())
            thread_.join();
    }
}

EventLoop* siren::net::EventLoopThread::startLoop()
{
    assert(!thread_.joinable() && "thread is running or can't join!");
    thread_ = std::thread([this] { threadFunc(); });
    EventLoop* loop = nullptr;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}
void siren::net::EventLoopThread::threadFunc()
{
    EventLoop loop;
    if (callback_) { // 先执行用户预设的回调函数
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_); //创建EventLoop并执行loop()
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
