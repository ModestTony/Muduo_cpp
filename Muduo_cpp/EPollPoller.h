#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * epoll的使用  
 * epoll_create
 * epoll_ctl 添加，修改，删除   add/mod/del updateChannel和removeChannel都是ctl方法
 * epoll_wait 对应poll
 */ 
class EPollPoller : public Poller
{

public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateChannel(Channel *channel) override;
    void removeChannel(Channel *channel) override;

private:
    static const int kInitEventListSize = 16; //epoll_event的数组长度

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    
    // 更新channel通道 epoll_ctl
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>; //c++中可以省略掉struct，直接写成epoll_event

    int epollfd_; // epoll_create创建返回的epoll句柄,保存在epollfd_上
    EventList events_; // 用于存放epoll_wait返回的所有发生的事件的文件描述符事件集合
};