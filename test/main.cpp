
#include <bits/stdc++.h>
#include <spdlog/logger.h>

#include <iostream>

#include "chrono"
#include "siren/base/Logger.h"
#include "siren/net/Buffer.h"
#include "siren/net/EventLoop.h"
#include "siren/net/EventLoopThread.h"
#include "siren/net/EventLoopThreadPool.h"
#include "siren/net/TcpClient.h"
#include "siren/net/TcpConnection.h"
#include "siren/net/TcpServer.h"
#include "siren/net/Timer.h"
#include"fmt/std.h"

using namespace std;
using namespace siren::net;

function<void()> f1 = []() { LOG_INFO("func1 call!!!!!!!!!!!"); };
function<void()> f2 = []() { LOG_INFO("func2 call!!!!!!!!!!!"); };
function<void()> f3 = []() { LOG_INFO("func3 call!!!!!!!!!!!"); };

void onConnection(const TcpConnectionPtr& conn) {
    if (conn->connected()) {
        conn->setTcpNoDelay(true);
    }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp) {
    conn->send(buf);
}

void test_loop() {
    siren::net::EventLoop loop;
    auto now = chrono::system_clock::now();
    auto after1s = now + chrono::seconds(1);
    auto after2s = now + chrono::seconds(2);
    loop.runAt(after2s, f3);
    loop.runAt(after1s, f2);
    loop.runAt(now, f1);
    loop.runEvery(1, f1);
    loop.loop();
}
void foo() { std::cout << "Hello from foo!" << std::endl; }

class Object{
    public:
        void voidFun(){
            cout<<"i am voidfun";
        }
        void test(){
            auto fb = std::bind(&Object::voidFun, this);
            function<void(int)> f = fb;
            f(1);
        }
};

int main() {
    Object o;
    o.test();
}
