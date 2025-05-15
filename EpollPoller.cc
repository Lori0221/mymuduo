#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

const int kNew = -1;    // 一个Channel还没添加到Poller， channel成员index_初始值
const int kAdded = 1;   // 已经添加到poller
const int kDeleted = 2; // channel已从poller中删除

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)   // 用vector创建epollEvents
{
    if(epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error: %d \n", errno);
    }
}

EpollPoller::~EpollPoller(){
    ::close(epollfd_);
}

// 对应epoll_wait
Timestamp EpollPoller::poll(int timeoutMs, ChannelList *activeChannels){
    // 高并发情况下，LOG_INFO会大量调用，影响性能，用LOG_DEBUG更合理
    LOG_INFO("Func=%s => fd total count:%lu\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    // LT 模式， 没上报的会一直上报
    if(numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);     // 2倍扩容
        }
    }
    else if(numEvents == 0)
    {
        LOG_DEBUG("%s timeout", __FUNCTION__);
    }
    else
    {
        // 其他错误
        if(saveErrno != EINTR){
            // 全局与局部
            errno = saveErrno;
            LOG_ERROR("EpollPoller::poll() err!");
        }
    }

    return now;
}


// channel 的 update 和 remove 通过调用Eventloop的updateChnenal 和 removeChannel
// 然后调用到了Poller的updateChannel 和 removeChannel
// 最后调用到了Epoll的updateChannel 和 removeChannel
void EpollPoller::updateChannel(Channel *channel){
    const int index = channel->index();     // 对应EpollPoller的三个状态
    LOG_INFO("Func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted){
        if(index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }else{
        // channel已经在poller上注册过了
        int fd = channel->fd();
        if(channel->isNoneEvent())
        {
            // 对任何事件都不感兴趣
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel
void EpollPoller::removeChannel(Channel *channel){
    int fd = channel->fd();
    channels_.erase(fd);
    LOG_INFO("Func=%s => fd=%d\n", __FUNCTION__, channel->fd());

    
    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接
void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const{
    for(int i = 0; i < numEvents; i++)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        // EventLoop就拿到了它的poller给它返回的所有事件的channel列表
        activeChannels->push_back(channel);
    }
}

// 更新Channel通道
// 对应epoll_ctr 的 add mod
void EpollPoller::update(int operation, Channel *channel){
    epoll_event event;
    bzero(&event, sizeof event);

    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl_add/mod error:%d\n", errno);
        }
    }
}