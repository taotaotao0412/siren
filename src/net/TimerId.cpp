#include "siren/net/TimerId.h"

siren::net::TimerId::TimerId() : timer_(nullptr), sequence_(0) {}

siren::net::TimerId::TimerId(Timer* timer, int64_t seq):timer_(timer), sequence_(seq) {}
