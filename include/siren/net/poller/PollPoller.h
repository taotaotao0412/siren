#pragma once
#include "siren/net/Channel.h"
#include "siren/net/EventLoop.h"
#include "siren/net/Poller.h"
#include "siren/net/Timer.h"

#include <vector>


namespace siren {
namespace net {
class PollPoller : public Poller {
   public:
    PollPoller(EventLoop* loop);
    ~PollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

   private:
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;

    typedef std::vector<struct pollfd> PollFdList;
    PollFdList pollfds_;
};

}  // namespace net

}  // namespace siren
