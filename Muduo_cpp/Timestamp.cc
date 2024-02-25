#include "Timestamp.h"

#include <time.h>

Timestamp::Timestamp():microSecondsSinceEpoch_(0) {} //默认构造函数，置0

Timestamp::Timestamp(int64_t microSecondsSinceEpoch): microSecondsSinceEpoch_(microSecondsSinceEpoch) {} //拷贝构造函数，赋值

Timestamp Timestamp::now(){ return Timestamp(time(NULL)); } // 获取当前系统的当前日期，时间 

std::string Timestamp::toString() const 
{
    char buf[128] = {0};
    tm *tm_time = localtime(&microSecondsSinceEpoch_); // man localtime 可以看到localtime会返回一个struct 
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
        tm_time->tm_year + 1900, tm_time->tm_mon + 1,tm_time->tm_mday,
        tm_time->tm_hour,tm_time->tm_min,tm_time->tm_sec);
    return buf; // buf就是时间的字符串格式
}

// #include <iostream>
// int main()
// {
//     std::cout << Timestamp::now().toString() << std::endl; 
//     return 0;
// }