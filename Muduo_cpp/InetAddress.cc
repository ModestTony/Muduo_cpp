#include "InetAddress.h"

#include <strings.h>
#include <string.h>

InetAddress::InetAddress(uint16_t port, std::string ip) 
{
    bzero(&addr_, sizeof addr_); //清零 memset
    //
    addr_.sin_family = AF_INET; // IPv4
    addr_.sin_port = htons(port); // htons 本地字节序转成网络字节序，大端小端之类的 h to ns 
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // ip.c_str 把ip转换成c语言字符串类型 sin_addr是结构体
}

std::string InetAddress::toIp() const
{
    // addr_
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf); //地址家族，
    return buf;
}

std::string InetAddress::toIpPort() const //返回127.0.0.1:8080
{
    // ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf); // 
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);  // n to hostshort 
}

// #include <iostream>
// int main()
// {
//     InetAddress addr(8080);
//     std::cout << addr.toIpPort() << std::endl;

//     return 0;
// }