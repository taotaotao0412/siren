#include "siren/net/Channel.h"
#include "siren/net/EventLoop.h"

#include <poll.h>

#include <sstream>

using namespace std;
using namespace siren::net;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

siren::net::Channel::Channel(EventLoop* loop, int fd_)
    : loop_(loop),
      fd_(fd_),
      events_(0),
      revents_(0),
      index_(-1),
      logHup_(true),
      tied_(false),
      eventHandling_(false),
      addedToLoop_(false) {}

siren::net::Channel::~Channel() {
    if (loop_->isInLoopThread()) {
        assert(!loop_->hasChannel(this));
    }
}

void siren::net::Channel::handleEvent(Timestamp receiveTime) {
    std::shared_ptr<void> guard;
    if (tied_) {
        guard = tie_.lock();
        if (guard) {
            handleEventWithGuard(receiveTime);
        }
    } else {
        handleEventWithGuard(receiveTime);
    }
}

void siren::net::Channel::setReadCallback(ReadEventCallback cb) {
    readCallback_ = std::move(cb);
}

void siren::net::Channel::setWriteCallback(EventCallback cb) {
    writeCallback_ = std::move(cb);
}

void siren::net::Channel::setCloseCallback(EventCallback cb) {
    closeCallback_ = std::move(cb);
}

void siren::net::Channel::setErrorCallback(EventCallback cb) {
    errorCallback_ = std::move(cb);
}

void siren::net::Channel::tie(const std::shared_ptr<void>& obj) {
    tie_ = obj;
    tied_ = true;
}

int siren::net::Channel::fd() const { return fd_; }

int siren::net::Channel::events() const { return events_; }

void siren::net::Channel::set_revents(int revt) { revents_ = revt; }

bool siren::net::Channel::isNoneEvent() const { return events_ == kNoneEvent; }

void siren::net::Channel::enableReading() {
    events_ |= kReadEvent;
    update();
}

void siren::net::Channel::disableReading() {
    events_ &= ~kReadEvent;
    update();
}

void siren::net::Channel::enableWriting() {
    events_ |= kWriteEvent;
    update();
}

void siren::net::Channel::disableWriting() {
    events_ &= ~kWriteEvent;
    update();
}

void siren::net::Channel::disableAll() {
    events_ = kNoneEvent;
    update();
}

bool siren::net::Channel::isWriting() const { return events_ & kWriteEvent; }

bool siren::net::Channel::isReading() const { return events_ & kReadEvent; }

int siren::net::Channel::index() const { return index_; }

void siren::net::Channel::set_index(int idx) { index_ = idx; }

std::string siren::net::Channel::reventsToString() const {
    return eventsToString(fd_, revents_);
}

std::string siren::net::Channel::eventsToString() const {
    return eventsToString(fd_, events_);
}

void siren::net::Channel::doNotLogHup() { logHup_ = false; }

siren::net::EventLoop* siren::net::Channel::ownerLoop() { return loop_; }

std::string siren::net::Channel::eventsToString(int fd, int ev) {
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN) oss << "IN ";
    if (ev & POLLPRI) oss << "PRI ";
    if (ev & POLLOUT) oss << "OUT ";
    if (ev & POLLHUP) oss << "HUP ";
    if (ev & POLLRDHUP) oss << "RDHUP ";
    if (ev & POLLERR) oss << "ERR ";
    if (ev & POLLNVAL) oss << "NVAL ";

    return oss.str();
}

void siren::net::Channel::update() {
    addedToLoop_ = true;
    loop_->updateChannel(this);
}

void siren::net::Channel::handleEventWithGuard(Timestamp receiveTime) {
    eventHandling_ = true;
    LOG_TRACE(reventsToString());
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        if (logHup_) {
            LOG_WARN("fd = {}, Channel::handle_event() POLLHUP", fd());
        }
        if (closeCallback_) closeCallback_();
    }
    if (revents_ & POLLNVAL) {
        LOG_WARN("fd = {}, Channel::handle_event() POLLNVAL", fd());
    }
    if (revents_ & (POLLERR | POLLNVAL)) {
        if (errorCallback_) errorCallback_();
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        if (readCallback_) readCallback_(receiveTime);
    }
    if (revents_ & POLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    eventHandling_ = false;
}

void siren::net::Channel::remove() {
    assert(isNoneEvent());
    addedToLoop_ = false;
    loop_->removeChannel(this);
}