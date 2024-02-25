#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");  //端口号+IP号 构造函数
    explicit InetAddress(const sockaddr_in &addr): addr_(addr) {}           //传入结构体，包含协议族

    std::string toIp() const; // 获取IP port信息
    std::string toIpPort() const; // 获取IP端口号
    uint16_t toPort() const; // 

    const sockaddr_in* getSockAddr() const {return &addr_;} //获取成员变量
    void setSockAddr(const sockaddr_in &addr) { addr_ = addr; } //修改成员变量
private:
    sockaddr_in addr_;  //  
};