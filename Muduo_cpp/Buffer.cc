#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据 Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */ 
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    /*
        在readFd( )函数中会在栈上创建一个临时空间extrabuf，然后使用readv的分散读特性，将TCP缓冲区中的数据先拷贝到Buffer中，
        如果Buffer容量不够，就把剩余的数据都拷贝到extrabuf中，然后再调整Buffer的容量(动态扩容)，再把extrabuf的数据拷贝到Buffer中。
        当这个函数结束后，extrabuf也会被释放。另外extrabuf是在栈上开辟的空间，速度比在堆上开辟还要快。
    */
    char extrabuf[65536] = {0}; // 栈上的内存空间大小 64K
    /*
        struct iovec {
            ptr_t iov_base // iov_base 指向的缓冲区存放的是readv接收的数据或者writev发送的数据
            size_t iov_len; // iov_len 在各种情况下分别确定接收最大长度以及实际写入长度
        }
        使用ioves 分配两个连续的缓冲区
        
    */
    struct iovec vec[2];
    
    const size_t writable = writableBytes(); // 可写缓冲区的大小

    
    vec[0].iov_base = begin() + writerIndex_; // 第一块缓冲区，指向可写空间
    vec[0].iov_len = writable;                // 当我们用readv从socket缓冲区读数据，首先会先填满这个vec[0],也就是我们的Buffer缓冲区
    
    vec[1].iov_base = extrabuf;               // 第二块缓冲区，如果Buffer缓冲区都填满了，那就填到我们临时创建的
    vec[1].iov_len = sizeof extrabuf;         // 栈空间上
    
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) //说明Buffer空间足够存了
    {
        writerIndex_ += n;
    }
    else // //Buffer空间不够存，需要把溢出的部分（extrabuf）倒到Buffer中（会先触发扩容机制）
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  //对buffer_扩容，将extrabuf存储的另一部分数据追加到buffer_上 writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}