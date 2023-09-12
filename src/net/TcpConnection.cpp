#include "siren/net/TcpConnection.h"

#include "siren/net/Buffer.h"
#include "siren/net/Channel.h"
#include "siren/net/EventLoop.h"
#include "siren/net/Socket.h"
#include "siren/net/SocketsOps.h"

using namespace siren::net;

void siren::net::defaultConnectionCallback(const TcpConnectionPtr& conn) {
    LOG_TRACE("{} -> {} is {}", conn->localAddress().toIpPort(),
              conn->peerAddress().toIpPort(),
              (conn->connected() ? "UP" : "DOWN"));
    // do not call conn->forceClose(), because some users want to register
    // message callback only.
}

void siren::net::defaultMessageCallback(const TcpConnectionPtr&, Buffer* buf,
                                        Timestamp) {
    buf->retrieveAll();
}

siren::net::TcpConnection::TcpConnection(EventLoop* loop, const string& name,
                                         int sockfd,
                                         const InetAddress& localAddr,
                                         const InetAddress& peerAddr)
    : loop_(loop),
      name_(name),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG("TcpConnection::ctor[{}] at {} fd = {}", name_, fmt::ptr(this),
              sockfd);
    socket_->setKeepAlive(true);
}

siren::net::TcpConnection::~TcpConnection() {
    LOG_DEBUG("TcpConnection::dtor[{}], fd = {}, state = {}", this->name_,
              this->channel_->fd(), this->stateToString());
}

bool siren::net::TcpConnection::getTcpInfo(tcp_info* info) const {
    return socket_->getTcpInfo(info);
}

std::string siren::net::TcpConnection::getTcpInfoString() const {
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}

void siren::net::TcpConnection::send(const void* message, int len) {
    if (len <= 0) return;
    if (state_ == kConnected) {
        if (loop_->isInLoopThread())
            sendInLoop(message, len);
        else {
            auto fp = static_cast<void (TcpConnection::*)(const void*, size_t)>(
                &TcpConnection::sendInLoop);

            loop_->runInLoop(std::bind(fp, this, message, len));
        }
    }
}

void siren::net::TcpConnection::send(const std::string& message) {
    send(message.data(), message.size());
}

void siren::net::TcpConnection::send(Buffer* buf) {
    if (state_ == kConnected && buf->readableBytes() > 0) {
        if (loop_->isInLoopThread()) {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        } else {
            auto fp = static_cast<void (TcpConnection::*)(const std::string&)>(
                &TcpConnection::sendInLoop);

            loop_->runInLoop(std::bind(fp,
                                       this, 
                                       buf->retrieveAllAsString()));
        }
    }
}

void siren::net::TcpConnection::shutdown() {
    if (state_ == kConnected) {
        setState(kDisconnecting);
        // $
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
    }
}

void siren::net::TcpConnection::forceClose() {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        loop_->queueInLoop(
            std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds) {
    if (state_ == kConnected || state_ == kDisconnecting) {
        setState(kDisconnecting);
        std::weak_ptr<TcpConnection> weakPtr = (shared_from_this());
        auto fp = [weakPtr, this]() {
            auto sharedPtr = weakPtr.lock();
            if (sharedPtr) sharedPtr->forceClose();
        };
        loop_->runAfter(seconds, fp);
    }
}

void siren::net::TcpConnection::setTcpNoDelay(bool on) {
    socket_->setTcpNoDelay(on);
}

void siren::net::TcpConnection::startRead() {
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void siren::net::TcpConnection::stopRead() {
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void siren::net::TcpConnection::connectEstablished() {
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}

void siren::net::TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if (state_ == kConnected) {
        setState(kDisconnected);
        channel_->disableAll();

        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

void siren::net::TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd());

    if (n > 0) {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0) {
        handleClose();
    } else {
        LOG_ERROR("TcpConnection::handleRead");
    }
}

void siren::net::TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if (channel_->isWriting()) {
        ssize_t n = sockets::write(channel_->fd(), outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if (n > 0) {                    // 写了点数据
            outputBuffer_.retrieve(n);  // 将readerIndex 向后移动n
            if (outputBuffer_.readableBytes() ==
                0) {  // 本次把所有的数据都写进socket了
                channel_->disableWriting();
                if (writeCompleteCallback_) {
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting) {
                    shutdownInLoop();
                }
            }
        } else {
            LOG_ERROR("TcpConnection::handleWrite");
        }
    }
}

void siren::net::TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_TRACE("fd = {}, state: {}", channel_->fd(), stateToString());
    assert(state_ == kDisconnecting || state_ == kConnected);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    // must be the last line
    closeCallback_(guardThis);
}

/**
 * @brief handle of error
 * @todo FIXME
 */
void siren::net::TcpConnection::handleError() {}

void siren::net::TcpConnection::sendInLoop(const std::string& message) {
    sendInLoop(message.data(), message.size());
}

void siren::net::TcpConnection::sendInLoop(const void* data, size_t len) {
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if (state_ == kDisconnected) {
        LOG_WARN("disconnected, give up writing");
        return;
    }

    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
        nwrote = sockets::write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_) {
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
        } else  // nwrote < 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE ||
                    errno == ECONNRESET)  
                {
                    faultError = true;
                }
            }
        }
    }
    if (!faultError && remaining > 0) {
        size_t oldLen = outputBuffer_.readableBytes();
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ &&
            highWaterMarkCallback_) {
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,
                                         shared_from_this(),
                                         oldLen + remaining));
        }
        outputBuffer_.append(static_cast<const char*>(data) + nwrote,
                             remaining);
        if (!channel_->isWriting()) {
            channel_->enableWriting();
        }
    }
}

void siren::net::TcpConnection::shutdownInLoop() {
    loop_->assertInLoopThread();
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceCloseInLoop() {
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting) {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}

const char* siren::net::TcpConnection::stateToString() const {
    switch (state_) {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::startReadInLoop() {
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading()) {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopReadInLoop() {
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading()) {
        channel_->disableReading();
        reading_ = true;
    }
}