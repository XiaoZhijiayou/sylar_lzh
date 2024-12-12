#include "fiber.h"
#include "config.h"
#include "macro.h"
#include <atomic>

namespace sylar{

static Logger::ptr g_logger = SYLAR_LOG_NEAME("system");
/**
 * 一个静态的、线程安全的原子变量，用于表示协程（fiber）的全局唯一 ID，其初始值为 0
 * */
static std::atomic<uint64_t> s_fiber_id {0};

static std::atomic<uint64_t> s_fiber_count {0};

/**
 * thread_local 是 C++11 引入的一个存储类修饰符，用来声明线程局部存储（Thread Local Storage, TLS）。每个线程都有自己独立的变量副本
 * */
static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr;

static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
    Config::Lookup<uint32_t>("fiber.stack_size",128 * 1024, "fiber stack size");


class MallocStackAllocator{
 public:
  static void* Alloc(size_t size){
    return malloc(size);
  }
  static void Dealloc(void* vp, size_t size){
    return free(vp);
  }
};

using StackAllocater = MallocStackAllocator;

Fiber::Fiber(){
  m_state = EXEC;
  SetThis(this);
  if(getcontext(&m_ctx)){
    SYLAR_ASSERT2(false,"getcontext");
  }
  ++s_fiber_count;
  SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}
Fiber::Fiber(std::function<void()> cb, size_t stack_size)
      :m_id(++s_fiber_id)
      ,m_cb(cb){
      ++s_fiber_count;
      m_stacksize = stack_size ? stack_size : g_fiber_stack_size->getValue();

      m_stack = StackAllocater ::Alloc(m_stacksize);
      if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getconnect");
      }
      m_ctx.uc_link = nullptr;
      m_ctx.uc_stack.ss_sp = m_stack;
      m_ctx.uc_stack.ss_size = m_stacksize;

      makecontext(&m_ctx,&Fiber::MainFunc,0);
}

Fiber::~Fiber(){
    --s_fiber_count;
    if(m_stack){
      SYLAR_ASSERT(m_state == TERM
                   || m_state == INIT);
      StackAllocater::Dealloc(m_stack,m_stacksize);
    }else{
      SYLAR_ASSERT(!m_cb);
      SYLAR_ASSERT(m_state == EXEC);
      Fiber* cur = t_fiber;
      if(cur == this){
        SetThis(nullptr);
      }
    }
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber id=" << m_id
                              << "total=" << s_fiber_count;
}

//重置协程函数，并重置状态
//INIT，TERM状态下调用
void Fiber::reset(std::function<void()> cb){

}
//切换到当前协程执行
void Fiber::swapIn(){

}
//切换到后台执行
void Fiber::swapOut(){

}

void SetThis(Fiber* f){

}

Fiber::ptr Fiber::GetThis(){

}
//协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady(){

}
//协程切换到后台，并且设置为Hold状态
void Fiber::YieldToHold(){

}
//总协程数
uint64_t Fiber::TotalFibers(){

}


void Fiber::MainFunc(){

}

}