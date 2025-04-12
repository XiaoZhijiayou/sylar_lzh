#include "scheduler.h"
#include <list>
#include <memory>
#include <vector>
#include "fiber.h"
#include "hook.h"
#include "log.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NEAME("system");

static thread_local Scheduler* t_scheduler = nullptr;

static thread_local Fiber* t_sheduler_fiber = nullptr;

Scheduler::Scheduler(size_t threads, bool use_caller, const std::string& name)
    : m_name(name) {
  SYLAR_ASSERT(threads > 0);

  // 如果使用主协程(当前线程)
  if (use_caller) {
    sylar::Fiber::GetThis(); // 获取当前协程
    --threads;  // 主协程已经占有一个线程
    SYLAR_ASSERT(GetThis() == nullptr); //确保当前没有调度器实例
    t_scheduler = this;
    
    // 创建root协程 调度协程(当前线程执行)
    // 绑定调度器核心调度函数 run
    m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    sylar::Thread::SetName(m_name);

    t_sheduler_fiber = m_rootFiber.get();  // 设置当前协程调度协程
    m_rootThread = sylar::GetThreadId();  // 获取当前线程id
    m_threadIds.push_back(m_rootThread);  // 添加到线程id数组
  } else {
    // use_call == false
    // 没有主协程的情况， rootThread = -1
    m_rootThread = -1;  // 未指定主线程
  }
  m_threadCount = threads;
}

Scheduler::~Scheduler() {
  // 确保调度器已经开始停止
  SYLAR_ASSERT(m_stopping);
  if (GetThis() == this) {
    t_scheduler = nullptr;  // 清空全局线程局部变量 t_scheduler
  }
}

Scheduler* Scheduler::GetThis() {
  return t_scheduler; // 获取当前线程的调度器
}

Fiber* Scheduler::GetMainFiber() {
  return t_sheduler_fiber;  // 获取当前线程的调度协程
}

/**
 * @brief 启动调度
 * @details 初始化调度线程池，如果只使用caller线程进行调度，那这个方法啥也不做
 */
void Scheduler::Start() {
  MutexType::Lock lock(m_mutex);
  if (!m_stopping) {
    return;
  }
  m_stopping = false;
  SYLAR_ASSERT(m_threads.empty());
  // 创建调度线程池
  m_threads.resize(m_threadCount);
  // 根据线程池大小 为每个线程创建一个调度线程，执行调度器的 run方法
  for (size_t i = 0; i < m_threadCount; ++i) {
    m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this),
                                  m_name + "_" + std::to_string(i)));
    m_threadIds.push_back(m_threads[i]->getId()); // 记录线程id
  }
  lock.unlock();
}

void Scheduler::Stop() {
  // 设置自动停止标志，表示调度器会在适当的时候停止
  m_autoStop = true;
  // 如果根协程存在，且线程数为 0，并且根协程已经结束或正在初始化
  if (m_rootFiber && m_threadCount == 0 &&
      (m_rootFiber->getState() == Fiber::TERM ||
       m_rootFiber->getState() == Fiber::INIT)) {
    SYLAR_LOG_INFO(g_logger) << this << "  stopped";
    m_stopping = true;  // 设置停止标志

    if (stopping()) {
      return; // 如果已经在停止过程中，直接返回
    }
  }
  // 如果当前线程是调度器的根线程，做相应的检查
  //bool exit_on_this_fiber = false;
  if (m_rootThread != -1) {
    SYLAR_ASSERT(GetThis() == this);  // 确保当前线程是调度器的根线程
  } else {
    SYLAR_ASSERT(GetThis() != this);  // 否则，当前线程不是调度器的根线程
  }
  m_stopping = true;  // 设置调度器正在停止

  // 唤醒所有调度线程
  for (size_t i = 0; i < m_threadCount; i++) {
    trickle();
  }
  if (m_rootFiber) {
    trickle();
  }
  if (m_rootFiber) {
    if (!stopping()) {
      m_rootFiber->call();
    }
  }

  // 等待所有线程退出
  std::vector<Thread::ptr> thrs;
  {
    MutexType::Lock lock(m_mutex);
    thrs.swap(m_threads); // 将线程池中的线程交换到 thrs
  }
  for (auto& i : thrs) {
    i->join();  // 等待所有线程退出
  }
}
void Scheduler::setThis() {
  t_scheduler = this;
}

/**
 * @brief 调度方法
 * @attention 每个调度线程的入口函数 主要任务是从协程队列中选择执行的程序，并执行他们
 * @details 当任务队列为空，代码会进入idle协程
 */
void Scheduler::run() {
  SYLAR_LOG_DEBUG(g_logger) << m_name << "run";
  set_hook_enable(true);
  // 设置当前调度器为全局调度器
  setThis();
  // 如果当前线程不是根线程，则将 t_scheduler_fiber 设置为当前协程
  if (sylar::GetThreadId() != m_rootThread) {
    t_sheduler_fiber = Fiber::GetThis().get();
  }

  // 创建一个空闲协程，用于没有可执行协程时的占位
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;  // 回调函数
  // 存储当前调度的协程和线程信息
  FiberAndThread ft;

  // 不停的从任务队列取任务执行
  while (true) {
    ft.reset(); // 重置协程和线程信息
    /// 指示任务队列是否还有未处理的任务。
    bool trickle_me = false;  // 标记是否需要唤醒
    /// 指示是否有线程处于忙碌状态。
    bool is_active = false;   // 标记是否有活跃的协程
    {
      MutexType::Lock lock(m_mutex);
      // 遍历所有待调度任务队列 寻找一个待执行的协程
      auto it = m_fibers.begin();
      while (it != m_fibers.end()) {
        // 检查线程是否匹配，如果不匹配则跳过
        if (it->thread != -1 && it->thread != sylar::GetThreadId()) {
          ++it;
          trickle_me = true;
          continue;
        }
        // 如果协程的状态是执行中，跳过当前协程
        SYLAR_ASSERT(it->fiber || it->cb);
        if (it->fiber && it->fiber->getState() == Fiber::EXEC) {
          ++it;
          continue;
        }
        // 找到一个可执行的协程
        ft = *it;
        m_fibers.erase(it++); // 从队列中移除
        ++m_activeThreadCount;  // 活跃线程数加 1
        is_active = true;
        break;
      }
      // 如果队列中有其他协程，则设置为需要唤醒
      trickle_me |= it != m_fibers.end();
    }
    if (trickle_me) {
      trickle();
    }
    // 有效协程
    if (ft.fiber && (ft.fiber->getState() != Fiber::TERM &&
                     ft.fiber->getState() != Fiber::EXCEPT)) {
      ft.fiber->swapIn(); // 执行协程
      --m_activeThreadCount;  // 活跃线程数减 1

      // 如果协程的状态为 READY，则重新调度它
      if (ft.fiber->getState() == Fiber::READY) {
        schedule(ft.fiber);
      } else if (ft.fiber->getState() != Fiber::TERM &&
                 ft.fiber->getState() != Fiber::EXCEPT) {
        ft.fiber->m_state = Fiber::HOLD;  // 将协程状态设置为 HOLD，保持在队列中
      }
      ft.reset();
    } 
    // 如果回调函数有效
    else if (ft.cb) {
      // 如果已有回调协程，重置它
      if (cb_fiber) {
        cb_fiber->reset(ft.cb);
      } else {
        // 否则创建一个新的回调协程
        cb_fiber.reset(new Fiber(ft.cb));
      }
      ft.reset(); // 重置协程和线程信息
      cb_fiber->swapIn(); // 执行回调协程
      --m_activeThreadCount;  // 活跃线程数减 1
      // 重新调用协程
      if (cb_fiber->getState() == Fiber::READY) {
        schedule(cb_fiber);
        cb_fiber.reset();
      } else if (cb_fiber->getState() == Fiber::EXCEPT ||
                 cb_fiber->getState() == Fiber::TERM) {
        cb_fiber->reset(nullptr);
      } else {
        cb_fiber->m_state = Fiber::HOLD;
        cb_fiber.reset();
      }
    }  
    // 如果没有可执行的协程 则进入 idle 协程
    else {
      if (is_active) {
        --m_activeThreadCount;
        continue;
      }
      // 空闲协程已终止 则退出调度
      if (idle_fiber->getState() == Fiber::TERM) {
        SYLAR_LOG_INFO(g_logger) << "idle fiber term";
        break;
      }
      ++m_idleThreadCount;  // 空闲线程数加1
      idle_fiber->swapIn(); // 执行空闲协程
      --m_idleThreadCount;  // 空闲线程数减1
       // 如果空闲协程没有终止或异常状态，设置为 HOLD
      if (idle_fiber->getState() != Fiber::TERM &&
          idle_fiber->getState() != Fiber::EXCEPT) {
        idle_fiber->m_state = Fiber::HOLD;
      }
    }
  }
}

void Scheduler::trickle() {
  SYLAR_LOG_INFO(g_logger) << "trickle";
}

// 判断调度器是否可以停止
bool Scheduler::stopping() {
  MutexType::Lock lock(m_mutex);
  // 自动停止 / 正在停止 / 子协程为空 / 无活跃线程
  return m_autoStop && m_stopping && m_fibers.empty() &&
         m_activeThreadCount == 0;
}

void Scheduler::idle() {
  SYLAR_LOG_INFO(g_logger) << "idle";
  while (!stopping()) {
    sylar::Fiber::YieldToHold();
  }
}

}  // namespace sylar