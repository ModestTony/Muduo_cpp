#pragma once

#include <iostream>
#include <string>

// 时间类
class Timestamp
{
public:
    Timestamp(); 
    explicit Timestamp(int64_t microSecondsSinceEpoch); //explicit 防止隐式构造转换
    static Timestamp now(); //静态方法
    std::string toString() const; //将时间戳转换成年月日时分秒的可视化string
private:
    int64_t microSecondsSinceEpoch_; //表示时间
};