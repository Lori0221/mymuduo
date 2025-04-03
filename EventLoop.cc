#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个Eventloop    thread_local机制
__thread EventLoop *t_loopInThisTread = nullptr;

const int kPollTimeMs = 10000;  // 默认的Poller I/O接口的超时时间

// 创建wakeupfd，用来通知唤醒subReactor处理新来的Channel
int createEventFd(){
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtfd < 0){
        LOG_FATAL("eventfd err: %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventFd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    // 判断线程是否已创建EventLoop
    if(t_loopInThisTread){
        LOG_FATAL("Another Eventloop %p exists in this thread %d \n", t_loopInThisTread, threadId_);
    }else{
        t_loopInThisTread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个EventLoop都将监听wakeupChannel的Epoll读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisTread = nullptr;
}

// 开启事件循环
void EventLoop::loop(){
    looping_ = true;
    quit_ = false;
    LOG_INFO("EventLoop %p start looping\n", this);

    while(!quit_){
        activeChannels_.clear();
        // 主要监听两类fd，client的fd和wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for(Channel* channel : activeChannels_){
            // poller监听哪些事件发生事件了，然后上报给Eventloop
            // 通知Channel处理相应事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作
        // mainLoop 实现注册一个回调cb（需要subloop来执行）
        // 通过wakeupfd唤醒subloop，然后subloop执行回调
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
}

// 退出事件循环
// 1. loop在自己的线程中调用quit，不会阻塞到poll中
// 2. 在非loop线程中，调用loop的quit
void EventLoop::quit(){
    quit_ = true;

    // 其他线程中调用quit，比如在subloop（worker）中调用了mainloop(IO)的quit
    if(!isInLoopTread()){
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb){
    if (isInLoopTread())    // 在当前loop线程中执行callback
    {
        cb();
    }else{
        // 在非当前loop线程中执行cb，需要唤醒loop所在线程执行cb
        queueInLoop(cb);
    }
    
}

// 把cb放入队列中，唤醒loop所在线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // 唤醒相应的需要执行上面回调操作的loop线程
    // 或 后面的意义是，当前线程正在执行回调，但是loop又有了新的回调
    if(!isInLoopTread() || callingPendingFunctors_){
        wakeup();   // 唤醒loop所在线程
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
    }
}

// 唤醒loop所在线程
// 向wakeupfd写一个数据, wakeupChannel就发生读事件， 当前loop线程就会被唤醒
void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法，调用poller的方法
void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel){
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        // 释放pendingFucntors, 应对高并发
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor& functor : functors){
        // 执行当前loop需要执行的回调操作
        functor();
    }

    callingPendingFunctors_ = false;
}