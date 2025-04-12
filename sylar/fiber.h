#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <ucontext.h>
#include <functional>
#include <memory>
#include "thread.h"

namespace sylar {

class Scheduler;

//使用【enable_shared_from_this】需要注意的是：
//1.不可以在MyClass的构造函数中使用shared_from_this()，因为父类的弱针指成员还没有被初始化。
//2.对象直接在栈上或堆上构建，也是不能使用shared_from_this()的，因为常规的构造，不会初始化-父类的弱指针，原理同上。
//3.如果要安全使用shared_from_this(），对象生存至始至终必须由shared_ptr管理，第一个shared_ptr将负责初始化那个父类的弱指针。花了一下午的时间才搞清楚，
//4.使用shared_from_this(）得到的智能指针，显然不是这个对象的第一个智能指针。
class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

 public:
  typedef std::shared_ptr<Fiber> ptr;

  enum Status {
    /// 初始化状态：协程刚被创建时的状态
    INIT,
    /// 暂停状态：协程A未执行完被暂停去执行其他协程，需要保存上下文信息
    HOLD,
    /// 执行中状态：协程正在执行的状态
    EXEC,
    /// 结束状态：协程运行结束
    TERM,
    /// 可执行状态
    READY,
    /// 异常状态
    EXCEPT
  };

 private:
 //将默认构造函数私有，不允许调用默认构造，由此推断出必然有一个自定义的公共有参构造函数
  Fiber();

 public:
 //自定义公共有参构造,提供回调方法和最大方法调用栈信息层数
  Fiber(std::function<void()> cb, size_t stack_size = 0,
        bool use_caller = false);
  ~Fiber();

  /// 重置协程函数，并重置状态，重置后该内存可以用在新协程的使用上，节约空间。
  /// INIT，TERM状态下调用
  void reset(std::function<void()> cb);
  /// 切换到当前协程执行
  void swapIn();
  /// 切换到后台执行
  void swapOut();
  //调用指定协程执行
  void call();

  void back();
  uint64_t getId() const { return m_id; }

  Status getState() const { return m_state; }

 public:
  /// 设置当前的协程
  static void SetThis(Fiber* f);
  /// 返回当前的协程
  static Fiber::ptr GetThis();
  /// 协程切换到后台，并且设置为Ready状态
  static void YieldToReady();
  /// 协程切换到后台，并且设置为Hold状态
  static void YieldToHold();
  /// 总协程数
  static uint64_t TotalFibers();

  /**
     * 协程执行函数
     * 执行完成返回到线程的主协程
     * */
  static void MainFunc();

  static void CallerMainFunc();

  static uint64_t GetFiberId();

 private:
  //协程ID
  uint64_t m_id = 0;
  //栈大小
  uint32_t m_stacksize = 0;
  //协程状态
  Status m_state = INIT;
  //协程上下文
  ucontext_t m_ctx;
  //协程栈
  void* m_stack = nullptr;
  //协程执行函数
  std::function<void()> m_cb;
};

}  // namespace sylar

#endif /* __SYLAR_FIBER_H__ */