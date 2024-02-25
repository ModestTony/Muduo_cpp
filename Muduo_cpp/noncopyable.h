#pragma once

/**
 * noncopyable被继承以后，派生类对象可以正常的构造和析构，但是派生类对象无法进行拷贝构造和赋值操作
 */ 
class noncopyable
{
public:
    noncopyable(const noncopyable&) = delete; //拷贝构造函数
    noncopyable& operator=(const noncopyable&) = delete; // 赋值运算符构造函数
protected:
    noncopyable() = default; //默认构造函数
    ~noncopyable() = default; //析构函数
};