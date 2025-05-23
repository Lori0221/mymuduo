#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中，多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    // 判断参数channel在当前poller当中
    virtual bool hasChannel(Channel *channel) const;

    // EentLoop可以通过该接口获取默认的IO复用的具体实现
    // 因为要return一个Poller指针，若在对应的cc里实现
    // 创建实例化对象要引入PollPoller.h 和 EpollPoller.h
    // 基类引入派生类头文件，这样实现不好
    static Poller* newDefaultPoller(EventLoop *loop);
protected:
    // （sockfd, channel类型）
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
private:
    EventLoop *ownerLoop_;  // 定义Poller所属的事件循环EventPoll
};
