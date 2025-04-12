#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <set>
#include <vector>
#include "mutex.h"
#include "thread.h"

namespace sylar {

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer> {
  friend class TimerManager;

 public:
  typedef std::shared_ptr<Timer> ptr;
  
  /**
   * 取消定时器，只是将m_next设置为0，并不会立即删除定时器，而是等到下次超时时再判断是否需要执行回调函数
   */
  bool cancel();

  /**
   * 刷新定时器，重新设置m_next，并将定时器插入到定时器管理器的定时器集合中
   */
  bool refresh();


  bool reset(uint64_t ms, bool from_now);

 private:

 /**
  * 构造函数只能由TimerManager创建Timer，构造函数有两个重载，一个是传入超时时间，一个是传入绝对时间。
  */
  Timer(uint64_t ms, std::function<void()> cb, bool recurring,
        TimerManager* manager);

  // 定时器比较仿函数
  struct Comparator {
    /**
      * @brief 重载()操作符，用于定时器的比较、排序
      * @details 比较两个定时器的执行时间，按照执行时间从小到大排序，如果执行时间相同，地址小的排在前面
      * @param[in] lhs 定时器指针_left hand side
      * @param[in] rhs 定时器指针_right hand side
      */
    bool operator()(const Timer::ptr lhs, const Timer::ptr& rhs) const;
  };

  Timer(uint64_t next);

 private:
  /// 是否循环定时器
  bool m_recurring = false;
  /// 执行周期
  uint64_t m_ms = 0;
  /// 精确的执行时间
  uint64_t m_next = 0;
  /// 回调函数
  std::function<void()> m_cb;
  /// 定时器管理器
  TimerManager* m_manager = nullptr;
};

class TimerManager {
  friend class Timer;

 public:
  typedef RWMutex RWMutexType;

  TimerManager();
  virtual ~TimerManager();

  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                      bool recurring = false);

  /**
   * 添加条件定时器，返回一个Timer对象。wear_ptr是一个弱引用，它不会增加所指对象的引用计数，也不会阻止所指对象被销毁。
   * 而shared_ptr是一种强引用，它会增加所指对象的引用计数，直到所有shared_ptr都被销毁才会释放所指对象的内存。
   * 在这段代码中，weak_cond是一个weak_ptr类型的对象，通过调用它的lock()方法可以得到一个shared_ptr类型的对象tmp，
   * 如果weak_ptr已经失效，则lock()方法返回一个空的shared_ptr对象。
   */
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb,
                               std::weak_ptr<void> weak_cond,
                               bool recuring = false);
  /**
   * 获取最近一个定时器执行的时间间隔，返回值为0表示已经超时，否则返回还要多久执行。
   */
  uint64_t getNextTimer();

  /**
   * @brief 获取需要执行的定时器的回调函数列表
   * @param[out] cbs 回调函数数组
   * */
  void listExpiredCb(std::vector<std::function<void()>>& cbs);
  /**
   * @brief 检测是否有定时器
   * */
  bool hasTimer();

 protected:
  virtual void onTimerInsertedAtFront() = 0;
  void addTimer(Timer::ptr val, RWMutexType::WriteLock& lock);

 private:

  /**
  * @brief 检查服务器时间是否被调后了
  * 检测是否发生了时间回拨，如果发生了时间回拨，需要将所有定时器视为过期。
  * */
  bool detectClockRollover(uint64_t now_ms);

 private:
  /// 锁
  RWMutexType m_mutex;
  /// 定时器的集合
  std::set<Timer::ptr, Timer::Comparator> m_timers;
  /// 是否触发onTimerInsertedAtFront
  bool m_trickled = false;
  /// 上次的执行时间
  uint64_t m_previouseTime = 0;
};

}  // namespace sylar

#endif