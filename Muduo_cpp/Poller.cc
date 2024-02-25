#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop): ownerLoop_(loop){}

bool Poller::hasChannel(Channel *channel) const{ //判断channel在poller是否存在
    auto it = channels_.find(channel->fd()); //查找sockfd，也就找到了对应的channel
    return it != channels_.end() && it->second == channel;
}