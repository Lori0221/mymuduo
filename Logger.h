#pragma once

// 库和项目分类，编程规范
#include <string>

#include "noncopyable.h"

// LOG_INFO("%s %d", arg1, arg2)
// ##__VA_ARGS__为获取可变参列表的宏
// 写宏的代码，多行要写换行; 为了防止错误，要用 do while(0)
#define LOG_INFO(logmsgFormat, ...) \
    do{ \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(INFO); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf); \
    }while(0)

#define LOG_ERROR(logmsgFormat, ...) \
    do{ \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(ERROR); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf); \
    }while(0)

#define LOG_FATAL(logmsgFormat, ...) \
    do{ \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(FATAL); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf); \
        exit(-1); \
    }while(0)

#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...) \
    do{ \
        Logger &logger = Logger::instance(); \
        logger.setLogLevel(DEBUG); \
        char buf[1024] = {0}; \
        snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__);  \
        logger.log(buf); \
    }while(0)
#else
    #define LOG_DEBUG(logmsgFormat, ...)
#endif

// 定义日志级别 
// INFO  打印重要流程信息
// ERROR 这些error不影响程序运行
// FATAL 毁灭性打击, core down
// DEBUG 调试，需要时才打开开关
enum LogLevel{
    INFO,
    ERROR,
    FATAL,
    DEBUG
};

// 数处一个日志类，根通信框架一样，写成单例
class Logger : noncopyable
{
public:
    // 获取日志唯一的实例对象
    static Logger& instance();
    // 设置日志级别
    void setLogLevel(int level);
    // 写日志
    void log(std::string msg);

private:
    int logLevel_;  // 系统变量下划线在前面，这样可以避免冲突
    Logger(){}
};
