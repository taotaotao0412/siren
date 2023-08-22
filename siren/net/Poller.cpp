#include "siren/net/Poller.h"
#include"siren/net/poller/EPollPoller.h"

using namespace siren::net;
siren::net::Poller::Poller(EventLoop* loop):ownerLoop_(loop) {}

bool siren::net::Poller::hasChannel(Channel* channel) const {
    assertInLoopThread();
    auto iter = channels_.find(channel->fd());
    return iter != channels_.end() && iter->second == channel;
}

Poller* siren::net::Poller::newDefaultPoller(EventLoop* loop) {
    return new EPollPoller(loop);
}

void siren::net::Poller::assertInLoopThread() const {
    ownerLoop_->assertInLoopThread();
}
