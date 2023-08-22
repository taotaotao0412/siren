#include "siren/net/poller/EPollPoller.h"
#include "siren/base/Logger.h"
#include "siren/net/Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>


using namespace siren;
using namespace siren::net;

static_assert(EPOLLIN == POLLIN, "epoll uses same flag values as poll");
static_assert(EPOLLPRI == POLLPRI, "epoll uses same flag values as poll");
static_assert(EPOLLOUT == POLLOUT, "epoll uses same flag values as poll");
static_assert(EPOLLRDHUP == POLLRDHUP, "epoll uses same flag values as poll");
static_assert(EPOLLERR == POLLERR, "epoll uses same flag values as poll");
static_assert(EPOLLHUP == POLLHUP, "epoll uses same flag values as poll");

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

siren::net::EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
    if (epollfd_ < 0) {
        LOG_ERROR("EPollPoller::EPollPoller");
    }
}
siren::net::EPollPoller::~EPollPoller() { close(epollfd_); }

Timestamp siren::net::EPollPoller::poll(int timeoutMs,
                                        ChannelList* activeChannels) {
    LOG_TRACE("fd total count {}", channels_.size());
    int numEvents = epoll_wait(epollfd_, events_.data(),
                               static_cast<int>(events_.size()), timeoutMs);
    auto now = std::chrono::system_clock::now();
    if (numEvents > 0) {
        LOG_TRACE("EPollPoller::poll: {} events happened", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size()) {
            events_.resize(events_.size() * 2);
        }
    } else if (numEvents == 0) {
        LOG_TRACE("nothing happened");
    } else {
        LOG_ERROR("EPollPoller::poll()");
    }
    return now;
}

void siren::net::EPollPoller::updateChannel(Channel* channel) {
    Poller::assertInLoopThread();

    const int index = channel->index();
    LOG_TRACE("fd = {}, events = {}, index = {}", channel->fd(),
              channel->events(), index);

    int fd = channel->fd();
    if (index == kNew || index == kDeleted) {
        if (index == kNew) {
            assert(channels_.find(fd) == channels_.end());
            channels_[fd] = channel;
        } else {
            assert(channels_.find(fd) != channels_.end());
            assert(channels_.find(fd)->second == channel);
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        if (channel->isNoneEvent()) {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void siren::net::EPollPoller::removeChannel(Channel* channel) {
    Poller::assertInLoopThread();
    int fd = channel->fd();
    LOG_TRACE("fd = {}", fd);
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(channel->isNoneEvent());
    int index = channel->index();
    assert(index == kAdded || index == kDeleted);
    size_t n = channels_.erase(fd);
    assert(n == 1);

    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void siren::net::EPollPoller::fillActiveChannels(
    int numEvents, ChannelList* activeChannels) const {
    for (int i = 0; i < numEvents; i++) {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

void siren::net::EPollPoller::update(int operation, Channel* channel) {
    struct epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();

    LOG_TRACE("epoll_ctl op = {}, fd = {}, event = {}",
              operationToString(operation), fd, channel->eventsToString());

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
        LOG_ERROR("epoll_ctl op = {}, fd = {}", operationToString(operation),
                  fd);
    }
}

const char* EPollPoller::operationToString(int op) {
    switch (op) {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}