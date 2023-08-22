#include "siren/net/Timer.h"

using namespace siren;
using namespace siren::net;

// std::atomic<long long> Timer::s_numCreated_;

std::atomic<long long> Timer::s_numCreated_;

void Timer::restart(std::chrono::system_clock::time_point now) {
    if (repeat_) {
        auto duration = std::chrono::milliseconds(static_cast<int64_t>(interval_ * 1000));
        expiration_ = now + duration;
    } else {
        expiration_ = std::chrono::system_clock::now();
    }
}
