#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller* Poller::newDefaultPoller(EventLoop *loop){
    if (::getenv("MUDUO_USE_POLL"))
    { //环境变量如果有MUDUO_USE_POLL，则使用poll
        return nullptr; // 生成poll的实例
    }
    else{
        return new EPollPoller(loop); // 生成epoll的实例，使用epoll
    }
}