#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

// 表示fd的状态
const int Channel::kNoneEvent = 0;    
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
    {}

Channel::~Channel(){}

// tie方法在哪里调用？ 连接一个TcpConnection新连接创建的时候 TcpConnection => Channel
void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示gd的events事件后，update在poller里更改fd相应的事件epoll_ctl
// EventLoop 里面
void Channel::update(){
    // 通过channel所属的eventloop， 调用poller相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的eventloop中删，每个eventloop包含一个poller和很多channel
void Channel::remove(){
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime){
    std::shared_ptr<void> guard;
    if(tied_){
        guard = tie_.lock();    // 提升为强智能指针
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller接收到的事件，由channel执行相应回调
void Channel::handleEventWithGuard(Timestamp receiveTime){
    // 打印log
    LOG_INFO("channel handleEvent revents:%d\n", revents_);


    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        if(closeCallback_){
            closeCallback_();   // 发生异常 
        }
    }

    if(revents_ & EPOLLERR){
        if(errorCallback_){
            errorCallback_();   // 发生错误
        }
    }

    if(revents_ & (EPOLLIN | EPOLLPRI)){
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }

    if(revents_ & EPOLLOUT){
        if(writeCallback_){
            writeCallback_();
        }
    }
}