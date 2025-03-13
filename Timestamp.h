#pragma once

#include <iostream>
#include <string>

class Timestamp
{
public:
    Timestamp();
    // 防止隐式转换
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string tostring() const;   // 只读
private:
    int64_t microSecondsSinceEpoch_;
};
