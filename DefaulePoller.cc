#include "Poller.h"
#include "EpollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop){
    if(::getenv("MUDUO_USE_POLL")){
        // 生成 Poll 的实例
        return nullptr;
    }else{
        // 生成 Epoll 的实例
        return new EpollPoller(loop);
    }
}