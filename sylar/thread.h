//
// Created by 11818 on 2024/12/10.
//

#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

//c++_11之前 pthread_xxx系列函数
//std::thread，pthread

#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include "mutex.h"
#include "noncopyable.h"

namespace sylar {

class Thread : Noncopyable {
 public:
  typedef std::shared_ptr<Thread> ptr;
  Thread(std::function<void()> cb, const std::string& name = "");
  ~Thread();
  pid_t getId() const { return m_id; }
  const std::string& getName() const { return m_name; }

  void join();

  static Thread* GetThis();
  static const std::string& GetName();
  static void SetName(const std::string& name);

 private:
  Thread(const Thread&) = delete;
  Thread(const Thread&&) = delete;
  Thread& operator=(const Thread&) = delete;
  Thread& operator=(const Thread&&) = delete;
  static void* run(void* arg);

 private:
  pid_t m_id = -1;
  pthread_t m_thread = 0;
  std::function<void()> m_cb;
  std::string m_name;
  //信号量
  Semaphore m_semaphore;
};

}  // namespace sylar

#endif  //SYLAR_THREAD_H
