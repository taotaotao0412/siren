#pragma once
#include "siren/net/Timer.h"

namespace siren {
namespace net {
class Timer;
class TimerId  {
   public:
    TimerId();
    TimerId(Timer *timer, int64_t seq);
    friend class TimerQueue;
    private:
    Timer *timer_;
    int64_t sequence_;
};
}  // namespace net

}  // namespace siren
