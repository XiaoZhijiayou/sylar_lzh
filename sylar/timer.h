#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include <vector>
#include <set>
#include "thread.h"
#include "mutex.h"

namespace sylar{

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>{
friend class TimerManager;
 public:
  typedef std::shared_ptr<Timer> ptr;

  bool cancel();
  bool refresh();
  bool reset(uint64_t ms,bool from_now);

private:
  Timer(uint64_t ms, std::function<void()> cb,
        bool recurring, TimerManager* manager);
  struct Comparator{
    bool operator()(const Timer::ptr lhs,const Timer::ptr& rhs) const;
  };
  Timer(uint64_t next);

 private:
  /// 是否循环定时器
  bool m_recurring = false;
  /// 执行周期
  uint64_t  m_ms = 0;
  /// 精确的执行时间
  uint64_t  m_next = 0;
  /// 回调函数
  std::function<void()> m_cb;
  /// 定时器管理器
  TimerManager* m_manager = nullptr;
};

class TimerManager{
friend class Timer;
public:
  typedef RWMutex RWMutexType;

  TimerManager();
  virtual ~TimerManager();

  Timer::ptr addTimer(uint64_t ms, std::function<void()> cb,
                      bool recurring = false);
  Timer::ptr addConditionTimer(uint64_t ms, std::function<void()> cb
                               ,std::weak_ptr<void> weak_cond
                               ,bool recuring = false);
  uint64_t getNextTimer();
  /**
   * @brief 获取需要执行的定时器的回调函数列表
   * @param[out] cbs 回调函数数组
   * */
  void listExpiredCb(std::vector<std::function<void()>>& cbs);
  bool hasTimer();
protected:
  virtual void onTimerInsertedAtFront() = 0;
  void addTimer(Timer::ptr val,RWMutexType::WriteLock& lock);

private:

  /**
  * @brief 检查服务器时间是否被调后了
  * */
  bool detectClockRollover(uint64_t now_ms);
private:
    /// 锁
    RWMutexType m_mutex;
    /// 定时器的集合
    std::set<Timer::ptr,Timer::Comparator> m_timers;
    /// 是否触发onTimerInsertedAtFront
    bool m_trickled = false;
    /// 上次的执行时间
    uint64_t m_previouseTime = 0;
};

}


#endif