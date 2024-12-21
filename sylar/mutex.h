#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__
#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace sylar {
class Semaphore {
 public:
  Semaphore(uint32_t count = 0);
  ~Semaphore();
  void wait();
  void notify();

 private:
  Semaphore(const Semaphore&) = delete;
  Semaphore(const Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&&) = delete;

 private:
  sem_t m_semaphore;
};

template <class T>
class SocpedLockImpl {
 public:
  SocpedLockImpl(T& mutex) : m_mutex(mutex) {
    m_mutex.lock();
    m_locked = true;
  }
  ~SocpedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_mutex;
  bool m_locked;
};

/**
 *  @brief 线程安全的读锁守护
 * */
template <class T>
class ReadSocpedLockImpl {
 public:
  ReadSocpedLockImpl(T& mutex) : m_mutex(mutex) {
    m_mutex.rdlock();
    m_locked = true;
  }
  ~ReadSocpedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_mutex;
  bool m_locked;
};

/**
 *  @brief 线程安全的写锁守护
 * */
template <class T>
class WriteSocpedLockImpl {
 public:
  WriteSocpedLockImpl(T& mutex) : m_mutex(mutex) {
    m_mutex.wrlock();
    m_locked = true;
  }
  ~WriteSocpedLockImpl() { unlock(); }

  void lock() {
    if (!m_locked) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock() {
    if (m_locked) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

 private:
  T& m_mutex;
  bool m_locked;
};

/**
 *  @brief 线程安全的互斥锁
 * */
class Mutex {
 public:
  typedef sylar::SocpedLockImpl<Mutex> Lock;
  Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

  ~Mutex() { pthread_mutex_destroy(&m_mutex); }

  void lock() { pthread_mutex_lock(&m_mutex); }

  void unlock() { pthread_mutex_unlock(&m_mutex); }

 private:
  pthread_mutex_t m_mutex;
};

class NullMutex {
 public:
  typedef sylar::SocpedLockImpl<NullMutex> Lock;

  NullMutex() {}

  ~NullMutex() {}

  void lock() {}

  void unlock() {}
};

/**
 *  @brief 线程安全的读写锁
 * */
class RWMutex {
 public:
  //局部读锁
  typedef ReadSocpedLockImpl<RWMutex> ReadLock;

  //局部写锁
  typedef WriteSocpedLockImpl<RWMutex> WriteLock;

  /**
   *  @brief 构造函数
   * */
  RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }

  /**
   *  @brief 析构函数
   * */
  ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

  /**
   *  @brief 读锁
   * */
  void rdlock() { pthread_rwlock_rdlock(&m_lock); }

  /**
   *  @brief 写锁
   * */
  void wrlock() { pthread_rwlock_wrlock(&m_lock); }

  /**
   *  @brief 解锁
   * */
  void unlock() { pthread_rwlock_unlock(&m_lock); }

 private:
  pthread_rwlock_t m_lock;
};

/**
 * @brief 空的读写锁
 * */
class NullRWMutex {
 public:
  //局部读锁
  typedef ReadSocpedLockImpl<NullRWMutex> ReadLock;

  //局部写锁
  typedef WriteSocpedLockImpl<NullRWMutex> WriteLock;

  NullRWMutex() {}

  ~NullRWMutex() {}

  void rdlock() {}

  void wrlock() {}

  void unlock() {}
};

class SpinLock {
 public:
  typedef SocpedLockImpl<SpinLock> Lock;
  SpinLock() { pthread_spin_init(&m_mutex, 0); }
  ~SpinLock() { pthread_spin_destroy(&m_mutex); }

  void lock() { pthread_spin_lock(&m_mutex); }

  void unlock() { pthread_spin_unlock(&m_mutex); }

 private:
  pthread_spinlock_t m_mutex;
};

class CASLock {
 public:
  typedef SocpedLockImpl<CASLock> Lock;

  CASLock() { m_mutex.clear(); }

  ~CASLock() {}

  void lock() {
    while (std::atomic_flag_test_and_set_explicit(&m_mutex,
                                                  std::memory_order_acquire))
      ;
  }

  void unlock() {
    std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
  }

 private:
  volatile std::atomic_flag m_mutex;
};

}  // namespace sylar

#endif /* __SYLAR_MUTEX_H__ */