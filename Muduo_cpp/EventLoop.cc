#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>


// 防止一个线程创建多个EventLoop  thread_local
__thread EventLoop *t_loopInThisThread = nullptr;
/*
    介绍一下这个__thread，这个__thread是一个关键字，被这个关键字修饰的全局变量 t_loopInThisThread会具备一个属性，
    那就是该变量在每一个线程内都会有一个独立的实体。因为一般的全局变量都是被同一个进程中的多个线程所共享，但是这里我们不希望这样。
    在EventLoop对象的构造函数中，如果当前线程没有绑定EventLoop对象，那么t_loopInThisThread为nullptr，
    然后就让该指针变量指向EventLoop对象的地址。如果t_loopInThisThread不为nullptr，说明当前线程已经绑定了一个EventLoop对象了，
    这时候EventLoop对象构造失败！
*/
// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来通知唤醒subReactor（eventloop）处理新来的channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); // eventfd文件描述符，并且该文件描述符设置为非阻塞和子进程不拷贝模式
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false) //初始没有开启循环
    , quit_(false)
    , callingPendingFunctors_(false) // 需要处理的回调
    , threadId_(CurrentThread::tid()) // 获取当前的线程id号
    , poller_(Poller::newDefaultPoller(this)) //获取默认的poller，即epoll
    , wakeupFd_(createEventfd())              // 创建wakeupfd，唤醒subreactor处理新来的channel
    , wakeupChannel_(new Channel(this , wakeupFd_)) //每个subreactor相当于一个eventloop，创建新事件？
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread) // 如果该线程已经创建了循环
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else // 该线程第一次创建循环
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupfd的事件类型以及发生事件后的回调操作。bind函数是一个绑定器         
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // 每一个eventloop都将监听wakeupchannel的EPOLLIN读事件了
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();   // 给channel移除所有感兴趣的事件，epoll就不会关注事件了
    wakeupChannel_->remove();       // 把channel从eventloop上移除
    ::close(wakeupFd_);             // 关闭wakeup文件描述符
    t_loopInThisThread = nullptr;   // 
}

// 开启事件循环
void EventLoop::loop()
{
    looping_ = true; //开始循环
    quit_ = false;   // 未退出
    LOG_INFO("EventLoop %p start looping \n", this); //事件开始运行
    while(!quit_) //while循环，
    {
        // 把eventloop的activatechannels传给epoll方法
        // 当epoll方法调用完epoll_wait()方法后，把有事件发生的channel装入activatechannels数组里面
        activeChannels_.clear();

        // 监听两类fd   一种是client的fd，即和客户端通信的fd，一种wakeupfd 即mainreactor和subreactor的通信fd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); 
        // 此时activateChannels中存放了经过epoll的epollwait函数记录的activatechannel
        for (Channel *channel : activeChannels_)
        {
            // 遍历activechannels，通知channel处理相应的事件（读，写，关闭事件），相当于调用channel的回调操作
            channel->handleEvent(pollReturnTime_);
        }
        /**
         * IO 线程mainLoop负责接收新用户的连接，accept fd -> channel subloop
         * Mainreactor接收新的fd，然后打包 分发给subreactor
         * mainLoop 事先注册一个回调cb（需要subloop来执行）    
         * wakeup subloop后，执行下面的方法，执行之前的mainloop注册的cb操作
         */ 
        doPendingFunctors(); //执行当前EventLoop事件循环需要处理的回调操作。
    }

    LOG_INFO("EventLoop %p stop looping. \n", this);
    looping_ = false;
}

// 退出事件循环  1.loop在自己的线程中调用quit  2.在非loop的线程中，调用loop的quit
/**
 *              mainLoop
 * 
 *                                             no ==================== 生产者-消费者的线程安全的队列
 * 
 *  subLoop1     subLoop2     subLoop3
 */ 
void EventLoop::quit()
{
    quit_ = true;

    // 如果是在其它线程中，调用的quit，在一个subloop(woker)中，调用了mainLoop(IO)的quit
    if (!isInLoopThread())  
    {
        wakeup();
    }
}

// 在当前loop中执行cb
void EventLoop::runInLoop(Functor cb) //callback
{
    if (isInLoopThread()) // 如果当前调用runInLoop的线程正好是EventLoop的运行线程，则直接执行此函数
    {
        cb();
    }
    else // 在非当前loop线程中执行cb , 就需要唤醒loop所在线程，执行cb
    {
        queueInLoop(cb);
    }
}
// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_); //加锁 
        pendingFunctors_.emplace_back(cb); // vector向队尾添加元素
    } //解锁

    // 唤醒相应的，需要执行上面回调操作的loop的线程
    // || callingPendingFunctors_的意思是：当前loop正在执行回调，但是loop又有了新的回调
    // 这时就要wakeup() loop所在线程，让它继续执行它的回调
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        /***
            这里还需要结合下EventLoop循环的实现，其中doPendingFunctors()是每轮循环的最后一步处理。 
            如果调用queueInLoop和EventLoop在同一个线程，且callingPendingFunctors_为false时，
            则说明：此时尚未执行到doPendingFunctors()。 那么此时即使不用wakeup，也可以在之后照旧
            执行doPendingFunctors()了。这么做的好处非常明显，可以减少对eventfd的io读写。
        ***/
        wakeup(); // 唤醒loop所在线程
        /***
            为什么要唤醒 EventLoop，我们首先调用了 pendingFunctors_.push_back(cb), 
            将该函数放在 pendingFunctors_中。EventLoop 的每一轮循环在最后会调用 
            doPendingFunctors 依次执行这些函数。而 EventLoop 的唤醒是通过 epoll_wait 实现的，
            如果此时该 EventLoop 中迟迟没有事件触发，那么 epoll_wait 一直就会阻塞。 
            这样会导致，pendingFunctors_中的任务迟迟不能被执行了。
            所以必须要唤醒 EventLoop ，从而让pendingFunctors_中的任务尽快被执行。
        ***/

    }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %lu bytes instead of 8", n);
  }
}

// 用来唤醒loop所在的线程的  向wakeupfd_写一个数据，wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

// EventLoop的方法 =》 Poller的方法
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors() // 执行回调
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);  //  释放pendingFunctors交到functors中去，提升效率，避免死锁
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }

    callingPendingFunctors_ = false;
}