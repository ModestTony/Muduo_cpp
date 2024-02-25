#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;  //线程初始化回调函数，上层回调

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), 
        const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
private:
    void threadFunc();

    EventLoop *loop_; //将线程和eventloop绑定
    bool exiting_;
    Thread thread_; //线程
    std::mutex mutex_; // 互斥锁
    std::condition_variable cond_; //
    ThreadInitCallback callback_; 
};