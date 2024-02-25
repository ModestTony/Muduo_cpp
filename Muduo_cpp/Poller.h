#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块，poller是一个基类，epoll是子类，负责重写poller类的纯虚函数
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>; // ChannelList是vector数组，存放了channel的指针类型

    Poller(EventLoop *loop); 
    virtual ~Poller() = default;

    // 给IO多路复用 poll/epoll 保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;
    
    // 判断一个Poller当中是否有这个channel
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现，poll和epoll都可以
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // unordered_map的key：sockfd  value：sockfd所属的channel通道类型的指针
    // unordered_map < sockfd，fd对应的channel >  
    // 当poller检测到某个套接字有事件发生时，通过文件描述符可以直接找到Channel,Channel里面有读回调写回调
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;  // 记录 文件描述符 -> Channel的映射，也帮忙保管所有注册在你这个Poller上的Channel

private:
    EventLoop *ownerLoop_; // 定义当前的Poller所属的事件循环EventLoop
};