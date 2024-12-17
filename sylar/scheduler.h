#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <list>
#include <memory>
#include <vector>
#include "fiber.h"
#include "mutex.h"
#include "thread.h"

namespace sylar {

class Scheduler {
 public:
  typedef std::shared_ptr<Scheduler> ptr;
  typedef Mutex MutexType;
  /**
   * @brief 构造函数
   * @param[in] threads 线程数量
   * @param[in] use_caller 是否使用当前调用线程
   * @param[in] name 协程调度器名称
   * */
  Scheduler(size_t threads = 1, bool use_caller = true,
            const std::string& name = "");
  virtual ~Scheduler();

  const std::string& getName() const { return m_name; }

  static Scheduler* GetThis();

  static Fiber* GetMainFiber();

  void Start();
  void Stop();

  template <class FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1) {
    bool need_trickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_trickle = scheduleNoLock(fc, thread);
    }
    if(need_trickle){
      trickle();
    }
  }

  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_trickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end){
        need_trickle = scheduleNoLock(&*begin) || need_trickle;
        ++begin;
      }
    }
    if(need_trickle){
      trickle();
    }
  }

 protected:
  virtual void trickle();
  void run();
  virtual bool stopping();
  virtual void idle();
  void setThis();
  /**
   * @brief 是否有空闲线程
   * */
  bool hasIdleThreads() {return m_idleThreadCount > 0;}
 private:
  template <class FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread = -1){
    bool need_trickle = m_fibers.empty();
    FiberAndThread ft(fc, thread);
    if(ft.fiber || ft.cb){
      m_fibers.push_back(ft);
    }
    return need_trickle;
  }

 private:
  struct FiberAndThread {
    Fiber::ptr fiber;
    std::function<void()> cb;
    //线程id
    int thread;

    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

    FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) { fiber.swap(*f); }

    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

    FiberAndThread(std::function<void()>* f, int thr) : thread(thr) {
      cb.swap(*f);
    }
    FiberAndThread() : thread(-1) {}

    void reset() {
      fiber = nullptr;
      cb = nullptr;
      thread = -1;
    }
  };

 private:
  MutexType m_mutex;
  /// 线程池
  std::vector<Thread::ptr> m_threads;
  /// 待执行的协程队列
  std::list<FiberAndThread> m_fibers;
  /// use_caller 为true时有效果，调度协程
  Fiber::ptr m_rootFiber;
  /// 协程调度器名称
  std::string m_name;
 protected:
  /// 协程下的线程id数组
  std::vector<int> m_threadIds;
  /// 线程数量
  size_t m_threadCount = 0;
  /// 工作线程数量
  std::atomic<size_t>  m_activeThreadCount = {0};
  /// 空闲线程数量
  std::atomic<size_t> m_idleThreadCount = {0};
  /// 是否正在停止
  bool m_stopping = true;
  /// 是否自动停止
  bool m_autoStop = false;
  /// 主线程id
  int m_rootThread = 0;
  };
}

#endif /* __SYLAR_SCHEDULER_H__ */