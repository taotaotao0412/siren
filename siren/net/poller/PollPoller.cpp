#include "siren/net/poller/PollPoller.h"

// #include <assert.h>
// #include <errno.h>
// #include <poll.h>


using namespace siren;
using namespace siren::net;

siren::net::PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}
siren::net::PollPoller::~PollPoller() {}

Timestamp siren::net::PollPoller::poll(int timeoutMs,
                                       ChannelList* activeChannels) {
    return Timestamp();
}

void siren::net::PollPoller::updateChannel(Channel* channel) {}

void siren::net::PollPoller::removeChannel(Channel* channel) {}

void siren::net::PollPoller::fillActiveChannels(
    int numEvents, ChannelList* activeChannels) const {}
