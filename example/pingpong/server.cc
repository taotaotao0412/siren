#include "siren/net/TcpServer.h"

#include "siren/base/Logger.h"
#include "siren/net/EventLoop.h"
#include "siren/net/InetAddress.h"
#include "fmt/std.h"

#include <utility>
#include<atomic>
#include <stdio.h>
#include <unistd.h>
#include<iostream>

using namespace std;
using namespace siren;
using namespace siren::net;

void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
{
  auto msg = buf->retrieveAllAsString();
  conn->send(msg);
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: server <address> <port> <threads>\n");
  }
  else
  {

    LOG_INFO("pid = {}, tid = {}", getpid(), fmt::format("{}", std::this_thread::get_id()));
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress listenAddr(ip, port);
    int threadCount = atoi(argv[3]);

    EventLoop loop;

    TcpServer server(&loop, listenAddr, "PingPong");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    if (threadCount > 1)
    {
      server.setThreadNum(threadCount);
    }

    server.start();

    loop.loop();
  }
}

