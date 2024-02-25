#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; 
const int Channel::kWriteEvent = EPOLLOUT;

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd): loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {}

Channel::~Channel() {}

// channel的tie方法什么时候调用？一个TcpConnection新连接创建的时候 TcpConnection => Channel 
void Channel::tie(const std::shared_ptr<void> &obj){
    tie_ = obj; //智能指针绑定
    tied_ = true;
}

/**
 * 当改变channel所表示fd的events事件后，update函数负责在poller里面更改fd相应的事件epoll_ctl
 * EventLoop => ChannelList   Poller
 * channellist和poller都属于evenetloop，update函数负责使用eventloop通知poller
 */ 
void Channel::update(){
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 和update方法相同，在channel所属的EventLoop中， 把当前的channel删除掉
void Channel::remove(){
    loop_->removeChannel(this);
}

// fd得到poller通知以后，处理事件的
void Channel::handleEvent(Timestamp receiveTime){
    if (tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if (guard){
            handleEventWithGuard(receiveTime);
        }
    }
    else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知channel发生的具体事件， 由channel负责调用具体的回调操作，即epoll返回的文件描述符被channel用来回调
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){ //发生异常，关闭连接
        if (closeCallback_){
            closeCallback_();
        }
    }

    if (revents_ & EPOLLERR){  //发生错误，执行错误处理
        if (errorCallback_){
            errorCallback_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)){ //如果有可读事件，
        if (readCallback_){
            readCallback_(receiveTime); //返回读事件
        }
    }

    if (revents_ & EPOLLOUT){ //如果有可写事件
        if (writeCallback_){
            writeCallback_(); //返回读事件
        }
    }
}