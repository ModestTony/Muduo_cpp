#pragma once //防止头文件重复包含

#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_ERROR(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 

#define LOG_FATAL(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
        exit(-1); \
    } while(0) 

#ifdef MUDEBUG //使用宏打开，如果定义了MUDEBUG，则执行，否则不执行
#define LOG_DEBUG(logmsgFormat, ...) \
    do \
    { \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
        logger.log(buf); \
    } while(0) 
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志的级别  INFO  ERROR  FATAL  DEBUG 
enum LogLevel
{
    INFO,  // 普通信息
    ERROR, // 错误信息
    FATAL, // core信息
    DEBUG, // 调试信息
};

// 输出一个日志类
class Logger : noncopyable //定义为不可拷贝的类
{
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);
private:
    int logLevel_;
};