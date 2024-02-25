#pragma once

#include <vector>
#include <string>
#include <algorithm>

// Buffer 封装了一个用户缓冲区，以及向这个缓冲区写数据读数据等一系列控制方法
// Buffer 类主要设计思想 (读写配合，缓冲区内部调整以及动态扩容）
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize) // 8 + 1024 头部+数据长度
        , readerIndex_(kCheapPrepend) // 预留偏移为8，防止出现粘包问题
        , writerIndex_(kCheapPrepend) 
    {}

    // 将缓冲区划分为3个部分，计算每个部分的长度
    size_t readableBytes() const { return writerIndex_ - readerIndex_; } // 
    size_t writableBytes() const { return buffer_.size() - writerIndex_;}
    size_t prependableBytes() const { return readerIndex_; }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const { return begin() + readerIndex_; }

    // onMessage string <- Buffer
    void retrieve(size_t len) // 从buffer中取长度为len的字节
    {
        if (len < readableBytes())
        {
            readerIndex_ += len; // 说明应用只读取了刻度缓冲区数据的一部分，就是len，还剩下readerIndex_ += len -> writerIndex_
        }
        else   // len == readableBytes() 复位操作
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回，把字符串全拿出来
    std::string retrieveAllAsString() // 获取缓冲区的所有数据，以string返回
    {
        return retrieveAsString(readableBytes()); // 应用可读取数据的长度
    }

    std::string retrieveAsString(size_t len) // 获取缓冲区长度为len的数据，以string返回
    {
        std::string result(peek(), len);
        retrieve(len); // 上面一句把缓冲区中可读的数据，已经读取出来，这里肯定要对缓冲区进行复位操作
        return result;
    }

    // 写入内容如果大于长度，则扩容 buffer_.size() - writerIndex_    len
    void ensureWriteableBytes(size_t len)
    { 
        if (writableBytes() < len)
        {
            makeSpace(len); // 扩容函数
        }
    }

    // 把 [data, data+len] 内存上的数据，添加到writable缓冲区当中
    void append(const char *data, size_t len)
    {
        ensureWriteableBytes(len);
        std::copy(data, data+len, beginWrite());
        writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + writerIndex_;
    }

    const char* beginWrite() const
    {
        return begin() + writerIndex_;
    }

    // 客户端发来数据，readFd从该TCP接收缓冲区中将数据读出来并放入到Buffer中。
    ssize_t readFd(int fd, int* saveErrno);
    
    // 服务端向这条TCP连接发送数据，通过该方法将Buffer中的数据拷贝到TCP发送缓冲区中。
    ssize_t writeFd(int fd, int* saveErrno);

private:
    char* begin()
    {
        // it.operator*()
        return &*buffer_.begin();  // vector底层数组首元素的地址，也就是数组的起始地址
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }
    // 8 + wirtablebytes + ... + perpendablebytes

    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readalbe = readableBytes();
            std::copy(begin() + readerIndex_, 
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readalbe;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};