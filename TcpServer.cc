#include "TcpServer.h"
#include "Logger.h"

#include <functional>

EventLoop *CheckLoopNotNull(EventLoop* loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}
    
TcpServer::TcpServer(EventLoop *loop,
    const InetAddress &listenAddr,
    const std::string &nameArg,
    Option option = kNoReusePort)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
{
    // 当有新用户连接时，会执行Tcp::newConnection回调
    acceptor_->setNewConentionCallback(std::bind(&TcpServer::newConnection, this,
    std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{

}