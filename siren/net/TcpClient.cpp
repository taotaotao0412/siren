#include "siren/net/TcpClient.h"

#include "TcpClient.h"

#include <utility>
#include "siren/base/Logger.h"
#include "siren/net/Callbacks.h"
#include "siren/net/Connector.h"
#include "siren/net/EventLoop.h"
#include "siren/net/SocketsOps.h"

using namespace siren;
using namespace siren::net;


namespace siren::net::detail {

    void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
        loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }

    void removeConnector(const ConnectorPtr &connector) {
        // connector->
    }

} // namespace siren::net::detail



siren::net::TcpClient::TcpClient(EventLoop *loop, const InetAddress &serverAddr,
                                 string nameArg)
        : loop_(loop),
          connector_(new Connector(loop, serverAddr)),
          name_(std::move(nameArg)),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          retry_(false),
          connect_(true),
          nextConnId_(1) {
    connector_->setNewConnectionCallback(
            std::bind(&TcpClient::newConnection, this, _1));
    // FIXME setConnectFailedCallback
    LOG_INFO("TcpClient::TcpClient[{}] - connector {}", name_,
             fmt::ptr(get_pointer(connector_)));
}

siren::net::TcpClient::~TcpClient() {
    LOG_INFO("TcpClient::~TcpClient[{}] - connector {}", name_,
             fmt::ptr(get_pointer(connector_)));
    TcpConnectionPtr conn;
    bool unique = false;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn) {
        assert(loop_ == conn->getLoop());
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique) {
            conn->forceClose();
        }
    } else {
        connector_->stop();
        // FIXME: HACK
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}

void siren::net::TcpClient::connect() {
    LOG_INFO("TcpClient::connect[{}] - connecting to {}", name_,
             connector_->serverAddress().toIpPort());
    connect_ = true;
    connector_->start();
}

void siren::net::TcpClient::disconnect() {
    connect_ = false;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void siren::net::TcpClient::stop() {
    connect_ = false;
    connector_->stop();
}

void siren::net::TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(),
             nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    InetAddress localAddr(sockets::getLocalAddr(sockfd));
    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    TcpConnectionPtr conn(
            new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));

    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
            std::bind(&TcpClient::removeConnection, this, _1));  // FIXME: unsafe
    {
        std::unique_lock<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

void siren::net::TcpClient::removeConnection(const TcpConnectionPtr &conn) {
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        std::unique_lock<std::mutex> lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_) {
        LOG_INFO("TcpClient::connect[{}] - Reconnecting to {}", name_,
                 connector_->serverAddress().toIpPort());
        connector_->restart();
    }
}
