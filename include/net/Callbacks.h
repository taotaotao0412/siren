#pragma once

// #include "siren/net/Timer.h"

#include <chrono>
#include <functional>
#include <memory>

namespace siren {
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

template <typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr) {
    return ptr.get();
}



namespace net {
class Buffer;
class TcpConnection;
using TimerCallback = std::function<void()>;
using Timestamp = std::chrono::system_clock::time_point;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

typedef std::function<void(const TcpConnectionPtr&, size_t)>
    HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>
    MessageCallback;
    
void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buffer,
                            Timestamp receiveTime);
}  // namespace net
}  // namespace siren
