#pragma once

#include <memory>
#include <string>

#include "EventLoop.h"

namespace KamaNet
{

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

// TcpConnection 代表一条已建立的 TCP 连接。
// 继承 enable_shared_from_this 以支持在成员函数内安全地生成指向自身的 shared_ptr，
// 从而确保对象在异步回调执行期间不会被提前析构。
class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name)
        : loop_(loop)
        , name_(name)
    {}

    ~TcpConnection() = default;

    const std::string& name() const { return name_; }
    EventLoop* getLoop() const { return loop_; }

    // connectDestroyed() 由 EventLoop 通过 queueInLoop 以 shared_ptr 方式调用，
    // 因此在此函数执行期间，至少有一个 shared_ptr（bind 表达式内部持有的那个）
    // 指向本对象，保证对象不会被提前销毁。
    // 当本函数返回后，bind 表达式持有的 shared_ptr 随 functor 析构而释放，
    // 若此时已无其他 shared_ptr 持有本对象，TcpConnection 对象才真正被销毁。
    void connectDestroyed()
    {
        // 执行连接销毁前的清理工作（此处为示意）
    }

private:
    EventLoop*  loop_;
    std::string name_;
};

} // namespace KamaNet
