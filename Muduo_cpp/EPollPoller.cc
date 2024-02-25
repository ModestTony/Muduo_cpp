#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// channel未添加到poller中
const int kNew = -1;  // channel的成员index_ = -1
// channel已添加到poller中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop): Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
events_(kInitEventListSize)  // vector<epoll_event> kInitEventListSize = 16;
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EPollPoller::~EPollPoller() {
    ::close(epollfd_);
}
/*
这个函数可以说是Poller的核心了，当外部调用poll方法的时候，
该方法底层其实是通过epoll_wait获取这个事件监听器上发生事件的fd及其对应发生的事件，
我们知道每个fd都是由一个Channel封装的，通过哈希表channels_可以根据fd找到封装这个fd的Channel。
将事件监听器监听到该fd发生的事件写进这个Channel中的revents成员变量中。然后把这个Channel装进activeChannels中（它是一个vector<Channel*>）。
这样，当外界调用完poll之后就能拿到事件监听器的监听结果（activeChannels_）
这个activeChannels就是事件监听器监听到的发生事件的fd，以及每个fd都发生了什么事件。
*/
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels){
    // 实际上应该用LOG_DEBUG输出日志更为合理
    LOG_INFO("func=%s => fd total count:%lu \n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs); //等待监听
    int saveErrno = errno;
    Timestamp now(Timestamp::now());
 
    if (numEvents > 0){ //大于0，表示有时间发生
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels); // 向activeChannels中填充
        if (numEvents == events_.size()){ // 扩容操作，如果事件达到最大值，扩容events，2倍扩容
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0){ //超时
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else{
        if (saveErrno != EINTR){
            errno = saveErrno;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller updateChannel removeChannel
/**
 *              EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 */ 
void EPollPoller::updateChannel(Channel *channel){
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), index);

    if (index == kNew || index == kDeleted){ //channel没有注册过或者被删除过了
        if (index == kNew){ // 
            int fd = channel->fd();
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else  // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->isNoneEvent()){ // 对任何事件不感兴趣
            update(EPOLL_CTL_DEL, channel); //删除channel
            channel->set_index(kDeleted); 
        }
        else{ //
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel
void EPollPoller::removeChannel(Channel *channel) 
{
    int fd = channel->fd();
    channels_.erase(fd); //unordered_map中

    LOG_INFO("func=%s => fd=%d\n", __FUNCTION__, fd);
    
    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

// 填写活跃的连接，向activeChannels中填充活跃事件
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{   
    for (int i=0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);//获取epoll中注册的channel，channel是活跃事件
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
    } // activeChannels中是所有的活跃事件
}

// 更新channel通道 epoll_ctl add/mod/del
void EPollPoller::update(int operation, Channel *channel) 
{ 
    //operation表示 add/mod/del
    epoll_event event;
    bzero(&event, sizeof event); //memset清空
    
    int fd = channel->fd();

    event.events = channel->events();
    event.data.fd = fd; 
    event.data.ptr = channel;
    
    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}