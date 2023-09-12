#pragma once

#include "siren/base/Logger.h"
#include "siren/base/noncopyable.h"
#include "siren/net/Timer.h"

#include <assert.h>
#include <functional>
#include <memory>
#include <string>

// #include "siren/net/EventLoop.h"
namespace siren {
namespace net {
    class EventLoop;
    class Channel : noncopyable {
    public:
        using EventCallback = std::function<void()>;
        using ReadEventCallback = std::function<void(Timestamp)>;

        /// @brief
        /// @param loop Pointer of EventLoop who handle the channel
        /// @param fd FD
        Channel(EventLoop* loop, int fd);
        ~Channel();

        void handleEvent(Timestamp receiveTime);
        void setReadCallback(ReadEventCallback cb);
        void setWriteCallback(EventCallback cb);
        void setCloseCallback(EventCallback cb);
        void setErrorCallback(EventCallback cb);


        void tie(const std::shared_ptr<void>&);

        int fd() const;
        int events() const;
        void set_revents(int revt); // used by pollers
        bool isNoneEvent() const;

        void enableReading();
        void disableReading();
        void enableWriting();
        void disableWriting();
        void disableAll();
        [[nodiscard]] bool isWriting() const;
        [[nodiscard]] bool isReading() const;

        // for Poller
        [[nodiscard]] int index() const;
        void set_index(int idx);

        // for debug
        std::string reventsToString() const;
        [[nodiscard]] std::string eventsToString() const;

        void doNotLogHup();

        EventLoop* ownerLoop();
        void remove();

    private:
        static std::string eventsToString(int fd, int ev);
        void update();
        void handleEventWithGuard(Timestamp receiveTime);

        static const int kNoneEvent;
        static const int kReadEvent;
        static const int kWriteEvent;

        EventLoop* loop_;
        const int fd_;
        int events_;
        int revents_; 
        int index_; 
        bool logHup_;

        std::weak_ptr<void> tie_;
        bool tied_;
        bool eventHandling_;
        bool addedToLoop_;
        ReadEventCallback readCallback_;
        EventCallback writeCallback_;
        EventCallback closeCallback_;
        EventCallback errorCallback_;
    };
} // namespace net

} // namespace siren
