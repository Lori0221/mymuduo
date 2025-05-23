#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>          
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err: %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking())
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    // TcpSever::start() Acceptor.listen 有新用户的连接，要执行一个回调
    // connfd -> channel -> subloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();     // 把acceptor注册到channel里
}

// listenfd有事件发生了，有新用户连接了
void Acceptor::handleRead()
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >= 0)
    {
        if(newConnectionCallback_)
        {
            // 轮询找到subloop，唤醒分发当前的新客户端的channel
            newConnectionCallback_(connfd, peerAddr);   
        }
        else
        {
            ::close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err: %d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if(errno == EMFILE)
        {
            // 可以调整文件描述符的上限，但往往意味着单独的服务器已无法满足需求
            LOG_ERROR("%s:%s:%d sockfd reacched limit!\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}