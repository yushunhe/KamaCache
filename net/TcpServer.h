#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "EventLoop.h"
#include "TcpConnection.h"

namespace KamaNet
{

// TcpServer 管理所有已建立的 TcpConnection 对象的生命周期。
class TcpServer
{
public:
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    explicit TcpServer(EventLoop* loop)
        : loop_(loop)
    {}

    ~TcpServer() = default;

    // 新连接到来时调用（通常由 Acceptor 回调触发）
    void newConnection(const std::string& name)
    {
        connections_[name] = std::make_shared<TcpConnection>(loop_, name);
    }

    // 移除指定连接（可从任意线程调用）
    void removeConnection(const TcpConnectionPtr& conn)
    {
        loop_->queueInLoop(
            std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    }

    // 在 EventLoop 线程中执行连接移除逻辑
    void removeConnectionInLoop(const TcpConnectionPtr& conn)
    {
        connections_.erase(conn->name());

        EventLoop* ioLoop = conn->getLoop();

        // *** 关键行 ***
        // 将 connectDestroyed 以 shared_ptr 方式绑定后投递到 ioLoop 队列。
        // bind 表达式内部持有一份 conn 的拷贝（即一个新的 shared_ptr），
        // 这就是"你看不到"的那个隐式 this——它在 connectDestroyed() 执行期间
        // 保证 TcpConnection 对象不被销毁。
        // 当 EventLoop::doPendingFunctors() 中该 functor 执行完毕、
        // functor 对象离开作用域被析构时，bind 内部的 shared_ptr<TcpConnection>
        // 引用计数减为 0，TcpConnection 对象才真正被彻底销毁。
        ioLoop->queueInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
        //  ↑ 这行代码（functor 析构时）释放了最后一个持有 TcpConnection 的 shared_ptr
    }

    const ConnectionMap& connections() const { return connections_; }

private:
    EventLoop*    loop_;
    ConnectionMap connections_;
};

} // namespace KamaNet
