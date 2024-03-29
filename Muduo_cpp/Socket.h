#pragma once

#include "noncopyable.h"

class InetAddress;

// 封装socket fd
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd): sockfd_(sockfd) {}
    ~Socket();
    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr); // 调用bind绑定服务器IP端口
    void listen(); // 调用listen监听套接字
    int accept(InetAddress *peeraddr); // 调用accept接收新客户连接请求
    void shutdownWrite(); // 调用shutdown关闭服务端写通道

    // 下面四个函数都是调用setsockopt设置一些socket选项
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_; // 服务器监听套接字文件描述符
};