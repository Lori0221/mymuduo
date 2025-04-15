#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
    const std::string &name = string())
    : loop_(nullptr)
    , exsiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exsiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    thread_.start();    // 启动底层新线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_ == nullptr){
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}

// 是在单独的新线程里运行的
void EventLoopThread::threadFunc()
{
    // one loop per thread
    EventLoop loop; // 创建一个独立的EventLoop, 和上面的线程一一对应

    if(callback_){
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    loop.loop();    // 开启了底层的Poller.poll()
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}