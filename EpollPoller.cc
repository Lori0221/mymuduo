#include "EpollPoller.h"
#include "Logger.h"

#include <errno.h>

const int kNew = -1;    // 一个Channel还没添加到Poller
const int kAdded = 1;   // 已经添加到poller
const int kDeleted = 2; // 已经关闭

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize)   // 用vector创建epollEvents
{
    if(epollfd_ < 0){
        LOG_FATAL("epoll_create error: %d \n", errno);
    }
}

EpollPoller::~EpollPoller(){
    ::close(epollfd_);
}