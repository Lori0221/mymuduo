#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;    // 前置类型，暴露少

/*
Channel 理解为通道
封装了sockfc和其感兴趣的event，如epollin，epollout事件
还绑定了poller返回的事件

EventLoop, Channel, Poller 在 Reactor模型上对应多路事件分发起， Demultiplex
*/

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(TimeStamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后，调用相应回调方法来处理事件
    void handleEvent(TimeStamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb){ readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&);

    int fd() const { return fd_; }
    int events() const { return events_; }
    int set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态，置位
    void enableReading() { events_ |= kReadEvent; update(); } 
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWritng() { events_ |= kWriteEvent; update(); }
    void disableWritng() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread, 每个channel都属于某个evetnloop
    EventLoop * owenerLoop() { return loop_; }
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    // 表示fd的状态
    static const int kNoneEvent;    
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_;   // 事件循环
    const int fd_;      // fd, poller监听的对象
    int events_;        // 注册fd感兴趣的事件
    int revents_;       // poller发生的具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为CHannel通道里面能够获知fd最终发生的事件revents
    // 所以它负责调用事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
    
};