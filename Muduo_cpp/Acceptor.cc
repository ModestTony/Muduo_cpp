#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <sys/types.h>    
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>


static int createNonblocking() // 静态方法，在accept中被调用
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblocking()) // socket
    , acceptChannel_(loop, acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    // TcpServer::start() Acceptor.listen  有新用户的连接，要执行一个回调（connfd=》channel=》subloop）
    // baseLoop => acceptChannel_(listenfd) => 
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this)); // 设置读回调
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll(); // 把从poller中感兴趣事件删除
    acceptChannel_.remove();// 调用eventlopp-》removechannel=》poller-》removechannel 把poller中channelmap对应部分删除
}

/*
listen()函数底层调用了linux的函数listen( )，开启对acceptSocket_的监听同时将acceptChannel及其感兴趣事件（可读事件）
注册到main EventLoop的事件监听器上。换言之就是让main EventLoop事件监听器去监听acceptSocket_
*/
void Acceptor::listen()
{
    listenning_ = true;
    acceptSocket_.listen(); // socket 中的 listen
    acceptChannel_.enableReading(); // acceptChannel_ => Poller
}

// handleRead() 接受新连接，并且以负载均衡的选择方式选择一个subEventLoop，并把这个新连接分发到这个subEventLoop上
void Acceptor::handleRead() // 有链接过来了
{
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            newConnectionCallback_(connfd, peerAddr); // 轮询找到subLoop，唤醒，分发当前的新客户端的Channel
        }
        else
        {
            ::close(connfd);
        }
    }
    else // 错误处理
    {
        LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE) // EMFILE 表示文件描述符达到上限
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}