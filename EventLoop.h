#pragma once


// 事件循环类，主要包含两个大模块， Channel 和 Poller (epoll的抽象)
class EventLoop
{
public:
    EventLoop();
    ~EventLoop();
private:
};
