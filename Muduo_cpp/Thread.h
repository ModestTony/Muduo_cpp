#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable 
{
public:
    using ThreadFunc = std::function<void()>; //线程函数

    explicit Thread(ThreadFunc, const std::string &name = std::string());
    ~Thread();

    void start(); //线程的启动
    void join(); //线程的销毁

    bool started() const { return started_; } //判断线程是否开启
    pid_t tid() const { return tid_; } //返回线程号
    const std::string& name() const { return name_; } // 

    static int numCreated() { return numCreated_; } //
private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;
};