#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
namespace sylar{

class IOManager : public Scheduler{
 public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef RWMutex RWMutexType;

  enum Event{
    /// 无事件发生
    NONE  = 0x0,
    /// 读事件(EPOLLIN)
    READ  = 0x1,
    /// 写事件(EPOLLOUT)
    WRITE = 0x4,
  };
 private:
  struct FdContext{
    typedef Mutex MutexType;

    struct EventContext{
      /// 事件执行的调度器
      Scheduler* scheduler = nullptr;
      /// 事件的协程
      Fiber::ptr fiber;
      /// 事件的回调函数
      std::function<void()> cb;
    };
    /// 事件相关的句柄
    int fd = 0;
    /// 读事件的上下文
    EventContext read;
    /// 写事件的上下文
    EventContext write;
    /// 当前的事件
    Event events = NONE;
    /// 事件的Mutex
    MutexType mutex;
  };

 public:
  IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");

  ~IOManager();
  /**
   * @brief 添加事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @param[in] cb    事件的回调函数
   * @return 添加事件成功返回0，失败返回-1
   * */
  int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
  /**
   * @brief 删除事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @attention 不会触发事件
   * */
  bool delEvent(int fd, Event event);

  /**
   * @brief 取消事件
   * @param[in] fd socket句柄
   * @param[in] event 事件类型
   * @attention 如果事件存在则触发事件
   * */
  bool cancelEvent(int fd, Event event);

  /**
   * @brief 取消所有事件
   * @param[in] fd socket句柄
   * */
  bool cacelAll(int fd);

  /**
   * @brief 返回当前的IOManager
   * */
   static IOManager* GetThis();

  protected:

   void trickle() override;
   bool stopping() override;
   void idle() override;

   void contextResize(size_t size);

  private:
   /// epoll 文件句柄
   int m_epfd = 0;
   /// pipe 文件句柄
   int m_trickleFds[2];
   /// 当前等待执行的事件数量
   std::atomic<size_t> m_pendingEventCount = {0};
   /// IOManager的Mutex
   RWMutexType m_mutex;
   /// socket事件上下文的容器
   std::vector<FdContext*> m_fdContexts;
};

}

#endif