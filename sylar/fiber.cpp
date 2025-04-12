#include "fiber.h"
#include <atomic>
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NEAME("system");
/**
 * 一个静态的、线程安全的原子变量，用于表示协程（fiber）的全局唯一 ID，其初始值为 0
 * */
static std::atomic<uint64_t> s_fiber_id{0};

static std::atomic<uint64_t> s_fiber_count{0};

/**
 * thread_local 是 C++11 引入的一个存储类修饰符，用来声明线程局部存储（Thread Local Storage, TLS）。每个线程都有自己独立的变量副本
 * */

/// 线程局部变量，当前线程正在运行的协程
/// t_fiber：保存当前正在运行的协程指针，必须时刻指向当前正在运行的协程对象。协程模块初始化时，t_fiber指向线程主协程对象。
static thread_local Fiber* t_fiber = nullptr;

/// 线程局部变量，当前线程的主协程，相当于切换到了主线程中运行，智能指针的形式
/// 保存线程主协程指针，智能指针形式。协程模块初始化时，t_thread_fiber指向线程主协程对象。
/// 当子协程resume时，通过swapcontext将主协程的上下文保存到t_thread_fiber的ucontext_t成员中，同时激活子协程的ucontext_t上下文。
/// 当子协程yield时，从t_thread_fiber中取得主协程的上下文并恢复运行。
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>(
    "fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
 public:
  static void* Alloc(size_t size) { return malloc(size); }
  static void Dealloc(void* vp, size_t size) { return free(vp); }
};

using StackAllocater = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
  if (t_fiber) {
    return t_fiber->getId();
  }
  return 0;
}

// ⽆参构造函数只⽤于创建线程的第⼀个协程，也就是线程主函数对应的协程
Fiber::Fiber() {
  m_state = EXEC;
  // 主协程设置为当前协程
  SetThis(this);
  // 将当前的上下文保存到m_ctx中
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  //  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << Fiber::GetFiberId();
  ++s_fiber_count;
  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// 有参构造函数，创建⼀个新的协程
Fiber::Fiber(std::function<void()> cb, size_t stack_size, bool use_caller)
    : m_id(++s_fiber_id), m_cb(cb) {
  ++s_fiber_count;
  // 若给了初始化值则用给定值，若没有则用约定值
  m_stacksize = stack_size ? stack_size : g_fiber_stack_size->getValue();
  
  // 获得协程运行指针
  m_stack = StackAllocater ::Alloc(m_stacksize);
  // 保存当前协程上下文信息到当前的Fiber中m_ctx中
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getconnect");
  }
  // uc_link置空，执行完当前context之后退出程序
  m_ctx.uc_link = nullptr;
  // 初始化栈指针
  m_ctx.uc_stack.ss_sp = m_stack;
  // 初始化栈大小
  m_ctx.uc_stack.ss_size = m_stacksize;

  if (!use_caller) {
    // 指明该context入口函数
    // 只有切换到这个m_ctx的时候才会执行这个函数
    // 就相当于只有对应的这个子协程被激活的时候才会执行这个函数
    makecontext(&m_ctx, &Fiber::MainFunc, 0);
  } else {
    makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
  }
  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

// 析构函数，用于释放协程的栈空间。
Fiber::~Fiber() {
  --s_fiber_count;
  // 根据栈内存是否为空，进行不同的释放操作
  if (m_stack) {
    // 有栈，子协程， 需确保子协程为结束状态
    SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
    // 释放运行栈
    StackAllocater::Dealloc(m_stack, m_stacksize);
  } else {
    // 无栈，主协程, 释放要保证没有任务并且当前正在运行
    SYLAR_ASSERT(!m_cb);
    SYLAR_ASSERT(m_state == EXEC);
    Fiber* cur = t_fiber;
    //若当前正在执行的协程为主协程，将当前协程置为空
    if (cur == this) {
      SetThis(nullptr);
    }
  }
  SYLAR_LOG_DEBUG(g_logger)
      << " Fiber::~Fiber id= " << m_id << " total= " << s_fiber_count;
}


//重置协程，就是重复利用已结束的协程，复用其栈空间，创建新协程
//INIT，TERM状态下调用
void Fiber::reset(std::function<void()> cb) {
  // 主协程不分配栈空间
  SYLAR_ASSERT(m_stack);
  // 当前协程在结束状态
  SYLAR_ASSERT(m_state == TERM || m_state == EXCEPT || m_state == INIT);
  m_cb = cb;
  if (getcontext(&m_ctx)) {
    SYLAR_ASSERT2(false, "getcontext");
  }
  m_ctx.uc_link = nullptr;
  // 将当前这个复用的协程的栈空间给与上之前的协程的栈空间
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;
  // 然后将当前协程的状态设置为READY，进入主线程中执行
  makecontext(&m_ctx, &Fiber::MainFunc, 0);
  m_state = INIT;
}

// 调用Fiber的执行
void Fiber::call() {
  SetThis(this);
  m_state = EXEC;
  // 将这个调用call的Fiber的上下文切换到t_threadFiber的上下文中
  if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

// 协程执行完之后，切换到调度的主协程
void Fiber::back() {
  SetThis(t_threadFiber.get());
  //保存当前上下文到oucp结构体中，然后激活upc上下文。
  if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

//切换到当前协程执行(一般是由主协程切换到子协程)
void Fiber::swapIn() {
  SetThis(this);
  SYLAR_ASSERT(m_state != EXEC);
  m_state = EXEC;
  if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

//切换到后台执行(一般是由子协程切换到主协程)
void Fiber::swapOut() {
  SetThis(Scheduler::GetMainFiber());
  if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
    SYLAR_ASSERT2(false, "swapcontext");
  }
}

//设置当前协程
void Fiber::SetThis(Fiber* f) {
  t_fiber = f;
}


// 返回当前协程如果当前线程还未创建协程，则创建线程的第一个协程，且该协程为当前线程的主协程，其他协程都通过这个协程来调度。
// 也就是说，其他协程结束时，都要切回到主协程，由主协程重新选择新的协程进行resume
Fiber::ptr Fiber::GetThis() {
  // 检查线程局部存储中是否存在活跃协程
  if (t_fiber) {
    // 返回当前协程的智能指针（通过 enable_shared_from_this）
    return t_fiber->shared_from_this();
  }
  // 如果当前没有活跃的协程，创建一个主协程
  // 首次调用时创建线程主协程（无栈协程）
  Fiber::ptr main_fiber(new Fiber);
  // 验证线程局部指针正确性（这里断言永远成立）
  SYLAR_ASSERT(t_fiber == main_fiber.get());
  // 保存主协程到线程局部存储（t_threadFiber 是智能指针）
  t_threadFiber = main_fiber;
  // 返回主协程的智能指针
  return t_fiber->shared_from_this();
}

//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->m_state == EXEC);
  cur->m_state = READY;
  cur->swapOut();
}

//协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur->m_state == EXEC);
  //  cur->m_state = HOLD;
  cur->swapOut();
}

//总协程数
uint64_t Fiber::TotalFibers() {
  return s_fiber_count;
}

// 协程的入口函数，用于执行协程的回调函数。
// 在用户传入的协程入口函数上进行了一次封装，这个封装类似于线程模块的对线程入口函数的封装。
void Fiber::MainFunc() {
  // 获取
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    cur->m_cb();
    cur->m_cb = nullptr;
    cur->m_state = TERM;
  } catch (std::exception& ex) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except:" << ex.what()
                              << " fiber_id=" << cur->getId() << std::endl
                              << sylar::BacktraceToString();
  } catch (...) {
    cur->m_state = EXCEPT;
    SYLAR_LOG_ERROR(g_logger)
        << "Fiber Except" << " fiber_id=" << cur->getId() << std::endl
        << sylar::BacktraceToString();
  }
  auto raw_ptr = cur.get();
  // 释放当前的t_fiber
  cur.reset();
  raw_ptr->swapOut();
  SYLAR_ASSERT2(false,
                "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

// 专为调度器自身协程设计，直接与线程主协程交互，减少调度器层级的上下文切换开销
void Fiber::CallerMainFunc() {
  // 获取当前协程上下文
  Fiber::ptr cur = GetThis();
  SYLAR_ASSERT(cur);
  try {
    // 执行协程任务
    cur->m_cb();
    // 清空任务回调
    cur->m_cb = nullptr;
    // 标记为终止状态
    cur->m_state = Fiber::TERM;
  } catch (std::exception& ex) {
    cur->m_state = Fiber::EXCEPT;
    SYLAR_LOG_ERROR(g_logger) << "Fiber Except: " << ex.what()
                              << " fiber_id =" << cur->getId() << std::endl
                              << sylar::BacktraceToString();
  } catch (...) {
    cur->m_state = Fiber::EXCEPT;
    SYLAR_LOG_ERROR(g_logger)
        << "Fiber Except" << " fiber_id =" << cur->getId() << std::endl
        << sylar::BacktraceToString();
  }
  auto raw_ptr = cur.get();
  // 解除智能指针绑定
  cur.reset();
  // 关键切换操作：切回调度器主协程
  raw_ptr->back();
  // 安全断言（永远不会执行到这里）
  SYLAR_ASSERT2(false,
                "never reach fiber_id=" + std::to_string(raw_ptr->getId()));
}

}  // namespace sylar