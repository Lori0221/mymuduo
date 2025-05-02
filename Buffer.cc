#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>

// 从fd上读取数据，Poller默认工作在LT模式
// Buffer缓冲区是有大小的，但是从fd上流式读数据时，不知道Tcp数据最终大小
ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    char extrabuf[65536] = {0};  // 栈上的内存空间, 64k
    struct iovec vec[2];

    // Buffer底层缓冲区剩余的可写空间大小
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof extrabuf;

    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable)     // buffer的可写缓冲区已经够存储读出来的数据
    {
        writerIndex_ += n;
    }
    else                        // extrabuf也写入了数据
    {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);  // writeIndex_开始写 n - writable大小的数据
    }

    return n;
}