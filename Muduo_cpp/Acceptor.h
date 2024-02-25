#pragma once
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

#include <functional>

class EventLoop;
class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb) 
    {
        newConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen();
private:
    void handleRead(); //回调函数
    
    EventLoop *loop_; // 这里的loop永远指向主reactor的eventloop，即testservers中创建的loop，Acceptor用的就是用户定义的那个baseLoop，也称作mainLoop
    Socket acceptSocket_; // 这个是服务器监听套接字的文件描述符
    Channel acceptChannel_; // 这是个Channel类，把acceptSocket_及其感兴趣事件和事件对应的处理函数都封装进去。
    NewConnectionCallback newConnectionCallback_; 
    // TcpServer构造函数中将TcpServer::newConnection( )函数注册给了这个成员变量。
    // 这个TcpServer::newConnection函数的功能是公平的选择一个subEventLoop，并把已经接受的连接分发给这个subEventLoop。
    bool listenning_;
};