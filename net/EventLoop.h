#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace KamaNet
{

// 事件循环，负责调度和执行回调任务
class EventLoop
{
public:
    using Functor = std::function<void()>;

    EventLoop() = default;
    ~EventLoop() = default;

    // 将任务加入待执行队列（线程安全）
    void queueInLoop(Functor cb)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    // 执行所有待执行的回调（由 EventLoop 线程调用）
    void doPendingFunctors()
    {
        std::vector<Functor> functors;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            functors.swap(pendingFunctors_);
        }
        // 逐一执行回调。每个 functor 执行完毕后，functor 对象本身随即析构，
        // 其中捕获的 shared_ptr 引用计数随之减少。
        // 这正是 TcpConnection 对象最终被销毁的那一行：
        //   ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
        // 当该 functor 执行完 connectDestroyed() 并在此处离开循环作用域时，
        // bind 表达式内部持有的 conn（shared_ptr<TcpConnection>）被析构，
        // 若此时已无其他 shared_ptr 持有该对象，TcpConnection 即被彻底销毁。
        for (Functor& functor : functors)
        {
            functor();  // ← 执行回调；functor 离开 for 作用域后析构，conn 随之释放
        }
    }

private:
    std::mutex              mutex_;
    std::vector<Functor>    pendingFunctors_;
};

} // namespace KamaNet
