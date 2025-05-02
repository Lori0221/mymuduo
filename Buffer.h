#pragma once

#include "noncopyable.h"

#include <vector>
#include <string>
#include <algorithm>

// 网络库底层的缓冲器类型定义
// kCheapPrepend | reader | writer
class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    explicit Buffer(size_t initalSize = kInitialSize)
        : buffer_(kCheapPrepend + initalSize)
        , readerIndex_(kCheapPrepend)
        , writerIndex_(kCheapPrepend)
    {}
    
    size_t readableBytes() const
    {
        return writerIndex_ - readerIndex_;
    }

    size_t writableBytes() const
    {
        return buffer_.size() - writerIndex_;
    }

    size_t prependableBytes() const
    {
        return readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址 
    const char* peek() const
    {
        return begin() + readerIndex_;
    }

    // onMessage string <- Buffer
    void retrieve(size_t len)
    {
        if(len < readableBytes())
        {
            // 应用只读取了可读缓冲区数据的一部分，即len长度
            // 还剩 readerIndex_ += len 到 writerIndex_没读
            readerIndex_ += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_ = writerIndex_ = kCheapPrepend;
    }

    // 把onMessage函数上报的Buffer数据，转成string类型的数据返回
    std::string retrieveAllAsString()
    {
        // 应用数据可读取的长度
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(), len);
        retrieve(len);  // 上一句把缓冲区中可读的数据读出来，这里对缓冲区进行复位
        return result;
    }

    // buffer_.size() - writerIndex_ 与 len 的关系
    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len)
        {
            // 扩容
            makeSpace(len);
        }
    }

    // 把[data， data+len]内存上的数据，添加到writable缓冲区当中
    void append(const char* data, size_t len)
    {
        ensureWritableBytes(len);
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

    // 从fd上读取数据
    ssize_t readFd(int fd, int *saveErrno);

private:
    char* begin()
    {
        // it.operator*().operator&()
        // vector底层数组的首元素的地址，也就是数组的起始地址
        return &*buffer_.begin();
    }
    const char* begin() const
    {
        return &*buffer_.begin();
    }

    void makeSpace(size_t len)
    {
        //
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            buffer_.resize(writerIndex_ + len);
        }
        else
        {
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex_,
                    begin() + writerIndex_,
                    begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};