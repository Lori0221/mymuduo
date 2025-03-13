#pragma once

/*
 noncopyable 被继承后，派生类对象可以正常进行构造和析构
 但是无法进行拷贝和赋值操作
*/

class noncopyable {
public:
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator= (const noncopyable&) = delete;  // 返回Void也可以
protected:  // 如果是private，派生类就不可访问了
    noncopyable() = default;
    ~noncopyable() = default;
};