#include "timer.h"
#include "util.h"

namespace sylar {

bool Timer::cancel() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (m_cb) {
    m_cb = nullptr;
    auto it = m_manager->m_timers.find(shared_from_this()); // 从定时器集合中查找当前定时器
    m_manager->m_timers.erase(it);
    return true;
  }
  return false;
}

bool Timer::refresh() {
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }
  auto it = m_manager->m_timers.find(shared_from_this()); // step1: 从定时器集合中查找当前定时器
  if (it == m_manager->m_timers.end()) {  
    return false;
  }                                     // 如果定时器不在定时器集合中，直接返回
  m_manager->m_timers.erase(it);  // step2: 从定时器集合中删除当前定时器
  m_next = sylar::GetCurrentMS() + m_ms;    // step3: 重新计算下次执行时间
  m_manager->m_timers.insert(shared_from_this()); // step4: 将当前定时器插入到定时器集合中
  return true;
}

bool Timer::reset(uint64_t ms, bool from_now) {
  if (ms == m_ms && !from_now) {    // 如果定时器执行周期和当前周期相同，并且不是从当前时间开始计算，已经是最新的定时器，直接返回true
    return true;
  }
  TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
  if (!m_cb) {
    return false;
  }   // 如果回调函数为空，直接返回
  auto it = m_manager->m_timers.find(shared_from_this());   // step1: 从定时器集合中查找当前定时器
  if (it == m_manager->m_timers.end()) {                                // 如果定时器不在定时器集合中，直接返回
    return false;
  }
  m_manager->m_timers.erase(it);                              // step2: 从定时器集合中删除当前定时器
  uint64_t start = 0;
  if (from_now) {
    start = sylar::GetCurrentMS();                                     // step3: 如果是从当前时间开始计算，直接使用当前时间
  } else {
    start = m_next - m_ms;                                              // 否则，重新计算执行时间
  }
  m_ms = ms;
  m_next = start + m_ms;                                                // step4: 重置的时间为开始时间+执行周期
  m_manager->addTimer(shared_from_this(), lock);                         // step5: 将当前定时器插入到定时器集合中
  return true;
}

Timer::Timer(uint64_t ms, std::function<void()> cb, bool recurring,
             TimerManager* manager)
    : m_recurring(recurring), m_ms(ms), m_cb(cb), m_manager(manager) {
  m_next = sylar::GetCurrentMS() + m_ms; 
}

// 传入绝对时间
Timer::Timer(uint64_t next) : m_next(next) {}

// 仿函数的实现
bool Timer::Comparator::operator()(const Timer::ptr lhs,
                                   const Timer::ptr& rhs) const {
  if (!lhs && !rhs) {
    return false;
  }
  if (!lhs) {
    return true;
  }
  if (!rhs) {
    return false;
  }
  if (lhs->m_next < rhs->m_next) {
    return true;
  }
  if (lhs->m_next > rhs->m_next) {
    return false;
  }
  return lhs.get() < rhs.get();
}

TimerManager::TimerManager() {
  m_previouseTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,
                                  bool recurring) {
  Timer::ptr timer(new Timer(ms, cb, recurring, this));   // 创建一个定时器，返回智能指针
  RWMutexType::WriteLock lock(m_mutex);
  addTimer(timer, lock);  // 添加定时器
  return timer;
}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb) {
  std::shared_ptr<void> tmp = weak_cond.lock();  /// 返回weak_cond的智能指针
  if (tmp) {
    cb();                       // 如果tmp有效，执行回调函数
  }  /// 如果没有释放，就执行cb回调函数
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,
                                           std::function<void()> cb,
                                           std::weak_ptr<void> weak_cond,
                                           bool recuring) {
  // 在定时器触发时会调用 OnTimer 函数，并在OnTimer函数中判断条件对象是否存在，如果存在则调用回调函数cb
  return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recuring);
}
uint64_t TimerManager::getNextTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  // 不触发 onTimerInsertedAtFront
  m_trickled = false;
  if (m_timers.empty()) {
    return ~0ull; // 如果定时器集合为空，返回最大值， ~0ull表示无符号长整型最大值
  }
  const Timer::ptr& next = *m_timers.begin(); // 获取定时器集合中最早执行的定时器
  uint64_t now_ms = sylar::GetCurrentMS();
  // 如果当前时间 >= 该定时器的执行时间，说明该定时器已经超时了，该执行了
  if (now_ms >= next->m_next) {
    return 0;
  } else {
    // 还没超时，返回还要多久执行
    return next->m_next - now_ms;
  }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs) {
  uint64_t now_ms = sylar::GetCurrentMS();
  std::vector<Timer::ptr> expired;
  {
    RWMutexType::ReadLock lock(m_mutex);
    if (m_timers.empty()) {
      return;
    }
  }

  RWMutexType::WriteLock lock(m_mutex);
  if (m_timers.empty()) {
    return;
  }

  bool rollower = detectClockRollover(now_ms);  // 是否发生了时间回拨
  // 如果没有发生时间回拨，并且第一个定时器都没有到执行时间，就说明没有任务需要执行
  if (!rollower && (*m_timers.begin())->m_next > now_ms) {
    return;
  }

  Timer::ptr now_timer(new Timer(now_ms));  // 以当前时间创建一个定时器
  // 若发生了时间回拨，则将m_timers的所有Timer视为过期
  // 否则返回第一个 >= now_ms的迭代器，在此迭代器之前的定时器全都已经超时
  auto it = rollower ? m_timers.end() : m_timers.lower_bound(now_timer);
  // 筛选出当前时间等于next时间执行的Timer
  while (it != m_timers.end() && (*it)->m_next == now_ms) {
    ++it;
  }
  // 小于当前时间的Timer都是已经过了时间的Timer，it是当前时间要执行的Timer
  // 将这些Timer都加入到expired中
  expired.insert(expired.begin(), m_timers.begin(), it);
  // 将已经放入expired的定时器删掉 
  m_timers.erase(m_timers.begin(), it);
  cbs.reserve(expired.size());      // 预留空间
  for (auto& timer : expired) {
    cbs.push_back(timer->m_cb);     // 将过期定时器的回调函数添加到回调函数集合中
    if (timer->m_recurring) {          // 如果是循环定时器，则再次放入定时器集合中
      timer->m_next = now_ms + timer->m_ms; // 重新计算下次执行时间
      m_timers.insert(timer);         // 将定时器重新添加到定时器集合中
    } else {  
      timer->m_cb = nullptr;          // 如果定时器不是重复执行的，将回调函数置为空
    }
  }
}

void TimerManager::addTimer(Timer::ptr val, RWMutexType::WriteLock& lock) {
  auto it = m_timers.insert(val).first;   // 将定时器添加到定时器集合中, .first表示返回一个pair，pair的first表示定时器的迭代器
  bool at_front = (it == m_timers.begin()) && !m_trickled;  // 判断是否是最早执行的定时器
  if (at_front) {
    m_trickled = true;      // 如果是最早执行的定时器，设置m_tickled为true
  }
  lock.unlock();            // 解锁
  if (at_front) {
    onTimerInsertedAtFront();   // 判断两次是为了在多线程环境中避免竞态条件，确保在解锁后，如果`at_front`仍为`true`
  }
}
bool TimerManager::detectClockRollover(uint64_t now_ms) {
  bool rollover = false;  // 如果当前时间比上次执行时间还小 并且 小于一个小时的时间，认为发生了时间回拨 
  if (now_ms < m_previouseTime && now_ms < (m_previouseTime - 60 * 60 * 1000)) {
    rollover = true;
  }
  m_previouseTime = now_ms;     // 重新更新时间
  return rollover;
}

bool TimerManager::hasTimer() {
  RWMutexType::ReadLock lock(m_mutex);
  return !m_timers.empty();
}

}  // namespace sylar