#pragma once

#include "siren/net/EventLoop.h"
#include "siren/net/Timer.h"

#include <map>
#include <vector>
#include <stdint.h>
#include <poll.h>


namespace siren {
namespace net {
class Channel;
using ChannelList = std::vector<Channel*>;
class EventLoop;
class Poller : noncopyable {
   public:
    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;

    /// Changes the interested I/O events.
    /// Must be called in the loop thread.
    virtual void updateChannel(Channel* channel) = 0;

    /// Remove the channel, when it destructs.
    /// Must be called in the loop thread.
    virtual void removeChannel(Channel* channel) = 0;

    virtual bool hasChannel(Channel* channel) const;

    static Poller* newDefaultPoller(EventLoop* loop);

    void assertInLoopThread() const;

   protected:
    typedef std::map<int, Channel*> ChannelMap;
    /**
     * @brief FD:int -> Channel pointer: Channel*
     */
    ChannelMap channels_;

   private:
    EventLoop* ownerLoop_;
};
}  // namespace net

}  // namespace siren
