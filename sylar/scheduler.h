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

  /**
   * @brief 析构函数
   * */
  virtual ~Scheduler();

  /**
   * @brief 获取协程调度器名称
   */
  const std::string& getName() const { return m_name; }

  /**
   * @brief 返回当前协程调度器
   */
  static Scheduler* GetThis();

  /**
   * 返回当前协程调度器的调度协程
   */
  static Fiber* GetMainFiber();

  /**
   * @brief 启动协程调度器
   */
  void Start();

  /**
   * @brief 停止协程调度器
   */
  void Stop();

  /**
   * @brief 单个协程调度
   * @details 将一个协程或回调函数 调度到执行队列中，并通知调度执行
   * @param fc 协程或回调函数
   * @param thread 协程执行的线程id -1表示 任意线程
   */
  template <class FiberOrCb>
  void schedule(FiberOrCb fc, int thread = -1) {
    bool need_trickle = false;
    {
      MutexType::Lock lock(m_mutex);
      need_trickle = scheduleNoLock(fc, thread);
    }
    if (need_trickle) {
      trickle();
    }
  }

  /**
   * @brief 批量调度协程
   * @details 将一组协程调度到执行队列中，并通知调度执行
   * @param begin 协程数组开始迭代器
   * @param end 协程数组结束迭代器
   */
  template <class InputIterator>
  void schedule(InputIterator begin, InputIterator end) {
    bool need_trickle = false;
    {
      MutexType::Lock lock(m_mutex);
      while (begin != end) {
        // 更新 need_tickle 若为 true，则说明有协程被调度
        need_trickle = scheduleNoLock(&*begin) || need_trickle;
        ++begin;
      }
    }
    if (need_trickle) {
      trickle();
    }
  }

 protected:
  virtual void trickle();
  void run();
  
  virtual bool stopping();

  /**
   * @brief 协程无任务可调用时执行 idle 协程
   */
  virtual void idle();
  
  /**
   * @brief 设置当前协程调度器
   */
  void setThis();
  /**
   * @brief 是否有空闲线程
   * */
  bool hasIdleThreads() { return m_idleThreadCount > 0; }

 private:
  /**
   * @brief 协程调度启动
   * @param fc 
   * @param thread
   * @return 
   */
  template <class FiberOrCb>
  bool scheduleNoLock(FiberOrCb fc, int thread = -1) {
    // 是否有协程待执行
    // 队列为空，唤醒调度器并传入协程
    // 队列不为空，已有协程任务，无需立即唤醒调度器
    bool need_trickle = m_fibers.empty();
    FiberAndThread ft(fc, thread); //保存协程对象和线程信息
    // 有效协程或回调函数，则加入队列
    if (ft.fiber || ft.cb) {
      m_fibers.push_back(ft);
    }
    return need_trickle;
  }

 private:
  struct FiberAndThread {
    /// 协程智能指针
    Fiber::ptr fiber;
    /// 协程执行函数
    std::function<void()> cb;
    //线程id
    int thread;

    /**
     * @brief 构造函数
     * @param f 协程
     * @param thr 线程id
     * @details 直接通过协程对象构造
     */
    FiberAndThread(Fiber::ptr f, int thr) : fiber(f), thread(thr) {}

    /**
     * @brief 构造函数
     * @param f 协程指针
     * @param thr 线程id
     * @post *f = nullptr
     * @details 从协程指针中获取协程对象，并把它转交给 fiber 成员
     */
    FiberAndThread(Fiber::ptr* f, int thr) : thread(thr) { fiber.swap(*f); }

    /**
     * @brief 构造函数
     * @param f 协程执行函数
     * @param thr 线程id
     * @details 通过回调函数构造
     */
    FiberAndThread(std::function<void()> f, int thr) : cb(f), thread(thr) {}

    /**
     * @brief 构造函数
     * @param f 协程执行函数指针
     * @param thr 线程id
     * @post *f = nullptr
     * @details 从回调函数指针中获取执行逻辑，并把它转交给 cb 成员
     */
    FiberAndThread(std::function<void()>* f, int thr) : thread(thr) {
      cb.swap(*f);
    }
    /**
     * @brief 无参构造函数
     * @details 创建空对象 -1 表示未指定线程
     */
    FiberAndThread() : thread(-1) {}

    /**
     * @brief 重置数据
     */
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
  std::atomic<size_t> m_activeThreadCount = {0};
  /// 空闲线程数量
  std::atomic<size_t> m_idleThreadCount = {0};
  /// 是否正在停止
  bool m_stopping = true;
  /// 是否自动停止
  bool m_autoStop = false;
  /// 主线程id
  int m_rootThread = 0;
};

}  // namespace sylar

#endif /* __SYLAR_SCHEDULER_H__ */