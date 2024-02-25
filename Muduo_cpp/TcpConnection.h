#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <memory>
#include <string>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 * TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
 * =》 TcpConnection 设置回调 =》 Channel =》 Poller =》 Channel的回调操作
 * 
 */ 
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, 
                const std::string &name, 
                int sockfd, // acceptr已连接的文件描述符
                const InetAddress& localAddr, // 本机地址 
                const InetAddress& peerAddr); // 对端地址
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; } // 判断是否已经简历连接

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();

    void setConnectionCallback(const ConnectionCallback& cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { messageCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

    void setCloseCallback(const CloseCallback& cb)
    { closeCallback_ = cb; }

    // 连接建立
    void connectEstablished();

    // 连接销毁
    void connectDestroyed();

private:
    enum StateE {kDisconnected, kConnecting, kConnected, kDisconnecting};
    void setState(StateE state) { state_ = state; }

    // 事件读写关闭错误，注册在channel上
    /*
        在一个已经建立好的Tcp连接上主要会发生四类事件：可读事件、可写事件、连接关闭事件、错误事件。
        当事件监听器监听到一个连接发生了以上的事件，那么就会在EventLoop中调用这些事件对应的处理函数。
    */
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // 这里绝对不是baseLoop， 因为TcpConnection都是在subLoop里面管理的
    const std::string name_; // 
    std::atomic_int state_; // 这个成员变量标识了当前TCP连接的状态（Connected、Connecting、Disconnecting、Disconnected）
    bool reading_; // 

    // 这里和Acceptor类似，Acceptor=》mainLoop    TcpConenction=》subLoop
    std::unique_ptr<Socket> socket_; // 用于保存已连接套接字文件描述符
    std::unique_ptr<Channel> channel_; 
    // channel_封装了上面的socket_及其各类事件的处理函数（读、写、错误、关闭等事件处理函数）。
    // 这个Channel种保存的各类事件的处理函数是在TcpConnection对象构造函数中注册的。

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    // 数据缓冲区
    Buffer inputBuffer_;  // inputBuffer_ 是一个Buffer类，是该TCP连接对应的用户接收缓冲区。
    Buffer outputBuffer_;
    /*
        outputBuffer_也是一个Buffer类，不过是用于暂存那些暂时发送不出去的待发送数据。因为Tcp发送缓冲区是有大小限制的，
        假如达到了高水位线，就没办法把发送的数据通过send()直接拷贝到Tcp发送缓冲区，而是暂存在这个outputBuffer_中，
        等TCP发送缓冲区有空间了，触发可写事件了，再把outputBuffer_中的数据拷贝到Tcp发送缓冲区中。
    */
};