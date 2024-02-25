#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * 理清楚 EventLoop、Channel、Poller之间的关系   ->  Reactor模型上对应 Demultiplex
 * Channel 理解为通道，封装了sockfd和其感兴趣的event，如EPOLLIN、EPOLLOUT事件
 * 还绑定了poller返回的具体事件
 */ 

class Channel : noncopyable //设置不可拷贝
{
public:
    using EventCallback = std::function<void()> ;                //事件回调，没有参数
    using ReadEventCallback = std::function<void(Timestamp)> ;   //读事件回调，参数是时间戳

    Channel(EventLoop *loop, int fd);
    ~Channel();
    /**
     * 当调用epoll_wait()后，可以得知事件监听器上哪些Channel（文件描述符）发生了哪些事件，
     * 事件发生后自然就要调用这些Channel对应的处理函数。HandleEvent()让每个发生了事件的Channel调用自己保管的事件处理函数。
     * 每个Channel会根据自己文件描述符实际发生的事件（通过Channel中的revents_变量得知）和感兴趣的事件（通过Channel中的events_变量得知）
     * 来选择调用read_callback_和/或write_callback_和/或close_callback_和/或error_callback_。
    */
    // fd得到poller通知以后，处理事件的
    void handleEvent(Timestamp receiveTime);   

    // 调用Event对应的事件处理器，设置回调函数对象
    // move用来表示一个对象t可以被“移动”，即允许资源从t有效地转移到另一个对象，即资源所有权转移，右值引用。
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止当channel被手动remove掉，channel还在执行回调操作
    void tie(const std::shared_ptr<void>&); //使用智能指针，防止防止当channel被手动remove掉，channel还在执行回调操作

    int fd() const { return fd_; } // 返回文件描述符
    int events() const { return events_; } // 返回我们关心的事件
    // 当事件监听器监听到某个文件描述符发生了什么事件，通过 set_revents() 函数可以将这个文件描述符实际发生的事件封装进这个Channel中
    int set_revents(int revt) { revents_ = revt; } 

    // 将Channel中的文件描述符及其感兴趣事件注册事件监听器上或从事件监听器上移除.
    // 外部通过这几个函数来告知Channel你所监管的文件描述符都对哪些事件类型感兴趣，
    // 并把这个文件描述符及其感兴趣事件注册到事件监听器（IO多路复用模块）上。
    // 这些函数里面都有一个update()私有成员方法，这个update其实本质上就是调用了epoll_ctl()
    void enableReading() { events_ |= kReadEvent; update(); }    // 让event对读事件感兴趣 update就是epoll_ctl函数
    void disableReading() { events_ &= ~kReadEvent; update(); }  // 让event对读事件不感兴趣
    void enableWriting() { events_ |= kWriteEvent; update(); }   
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // one loop per thread 返回属于哪个eventloop，实现多路复用
    EventLoop* ownerLoop() { return loop_; }
    void remove();
private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime); //处理受保护的处理事件

    // 这三个变量代表fd对哪些事件类型感兴趣
    static const int kNoneEvent;  // 0 任何事件都不感兴趣
    static const int kReadEvent;  // EPOLLIN | EPOLLPRI 记录对读事件感兴趣
    static const int kWriteEvent; // EPOLLOUT 记录写事件感兴趣

    EventLoop *loop_; // 记录这个channel属于哪个eventloop对象
    const int fd_;    // fd, Poller监听的对象
    int events_;      // fd感兴趣的事件集合
    int revents_;     // 代表事件监听器实际监听到该fd发生的事件类型集合，当事件监听器监听到一个fd发生了什么事件，通过Channel::set_revents()函数来设置revents值。
    int index_;       // 这个index_其实表示的是这个channel的状态，是kNew还是kAdded还是kDeleted
                      // kNew代表这个channel未添加到poller中，kAdded表示已添加到poller中，kDeleted表示从poller中删除了

    std::weak_ptr<void> tie_;  //绑定。。。TCPConnection
    bool tied_;                // 延长生命周期，保证按照顺序释放

    // 这些是std::function类型，代表着这个Channel为这个文件描述符fd保存的各事件类型发生时的处理函数。比如这个fd发生了可读事件，
    // 需要执行可读事件处理函数，这时候Channel类都替你保管好了这些可调用函数，要用执行的时候直接管保姆要就可以了。
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};

