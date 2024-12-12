#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

  enum Status {
    INIT,
    HOLD,
    EXEC,
    TERM,
    READY
  };

private:
    Fiber();
public:
    Fiber(std::function<void()> cb, size_t stack_size = 0);
    ~Fiber();
    //重置协程函数，并重置状态
    //INIT，TERM状态下调用
    void reset(std::function<void()> cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

public:
    //设置当前的协程
    static void SetThis(Fiber* f);
    //返回当前的协程
    static Fiber::ptr GetThis();
    //协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    //协程切换到后台，并且设置为Hold状态
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();

    /**
     * 协程执行函数
     * 执行完成返回到线程的主协程
     * */
    static  void MainFunc();

    /**
     *
     * */
private:
    //协程ID
    uint64_t m_id = 0;
    //栈大小
    uint32_t m_stacksize = 0;
    //协程状态
    Status m_state = INIT;
    //协程上下文
    ucontext_t m_ctx;
    //协程栈
    void* m_stack = nullptr;
    //协程执行函数
    std::function<void()> m_cb;
};


}


#endif /* __SYLAR_FIBER_H__ */