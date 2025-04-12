#include "hook.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "fd_manager.h"
#include "iomanager.h"
#include "log.h"
sylar::Logger::ptr g_logger = SYLAR_LOG_NEAME("system");
namespace sylar {

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
  XX(sleep)          \
  XX(usleep)         \
  XX(nanosleep)      \
  XX(socket)         \
  XX(connect)        \
  XX(accept)         \
  XX(read)           \
  XX(readv)          \
  XX(recv)           \
  XX(recvfrom)       \
  XX(recvmsg)        \
  XX(write)          \
  XX(writev)         \
  XX(send)           \
  XX(sendto)         \
  XX(sendmsg)        \
  XX(close)          \
  XX(fcntl)          \
  XX(ioctl)          \
  XX(getsockopt)     \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }
// dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
// RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
// 取出原函数，赋值给新函数
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;

struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();

    g_tcp_connect_timeout->addListener(
        [](const int& old_value, const int& new_value) {
          SYLAR_LOG_INFO(g_logger) << "tcp connect timeout changed from"
                                   << old_value << " to " << new_value;
          s_connect_timeout = new_value;
        });
  }
};

static _HookIniter s_hook_initer;

bool is_hook_enable() {
  return t_hook_enable;
}

void set_hook_enable(bool flag) {
  t_hook_enable = flag;
}

}  // namespace sylar

struct timer_info {
  int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                     uint32_t event, int timeout_so, Args&&... args) {
  // 如果不hook，直接返回原接口
  if (!sylar::t_hook_enable) {
    // forward完美转发，可以将传入的可变参数args以原始类型的方式传递给函数fun。
    // 这样做的好处是可以避免不必要的类型转换和拷贝，提高代码的效率和性能。
    return fun(fd, std::forward<Args>(args)...);
  }
  // 获取fd对应的FdCtx
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if (!ctx) {
    // 没有文件
    return fun(fd, std::forward<Args>(args)...);
  }
  // 检查句柄是否关闭
  if (ctx->isClose()) {
    errno = EBADE;
    return -1;
  }
  // 如果不是socket或者用户设置了非阻塞，直接调用原始函数
  if (!ctx->isSocket() || ctx->getUserNonblock()) {
    return fun(fd, std::forward<Args>(args)...);
  }
  uint64_t to = ctx->getTimeout(timeout_so);  // 获取超时时间
  std::shared_ptr<timer_info> tinfo(new timer_info);  // 设置超时条件

retry:
  // 先调用原始函数读数据或写数据 若函数返回值有效就直接返回
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  // 若中断则重试  Interrupted system call  用于那些可能永远阻塞的系统调用 
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  // 若为阻塞状态
  // 这种情况就是没有数据可读或没有空间可写
  // 此时需要设置非阻塞，然后调用epoll_wait等待事件
  if (n == -1 && errno == EAGAIN) {
    SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
    sylar::IOManager* iom = sylar::IOManager::GetThis();  // 获取IOManager
    sylar::Timer::ptr timer;  // 定时器
    std::weak_ptr<timer_info> winfo(tinfo); // tinfo的弱指针，可以判断tinfo是否已经销毁
    // 若超时时间不为-1，则设置定时器
    if (to != (uint64_t)-1) {
      // 添加条件定时器 —— to时间消息还没来就触发callback 
      timer = iom->addConditionTimer(
          to,
          [winfo, fd, iom, event]() {
            auto t = winfo.lock();
            // tinfo失效 || 设了错误   定时器失效了
            if (!t || t->cancelled) {
              return;
            }
            // 操作超时
            t->cancelled = ETIMEDOUT; // 没错误的话设置为超时而失败
            iom->cancelEvent(fd, (sylar::IOManager::Event)(event)); 
          },
          winfo);// 取消事件强制唤醒
    }
    // 添加事件
    int rt = iom->addEvent(fd, (sylar::IOManager::Event)(event));
    // 添加事件失败
    if (rt) {
      SYLAR_LOG_ERROR(g_logger)
          << hook_fun_name << "addEvent(" << fd << ", " << event << ")";
      if (timer) {
        timer->cancel();  // 取消定时器
      }
      return -1;
    } else {
       /*	addEvent成功，把执行时间让出来
        *	只有两种情况会从这回来：
        * 	1) 超时了， timer cancelEvent triggerEvent会唤醒回来
        * 	2) addEvent数据回来了会唤醒回来 */
      SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
      sylar::Fiber::YieldToHold();
      SYLAR_LOG_DEBUG(g_logger) << "do_io<" << hook_fun_name << ">";
      if (timer) {
        timer->cancel();      // 回来了还有定时器就取消掉
      }
      if (tinfo->cancelled) {   // 从定时任务唤醒，超时失败
        errno = tinfo->cancelled;
        return -1;
      }
      // 数据来了就直接重新去操作
      goto retry;
    }
  }
  return n;
}

extern "C" {
#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUN(XX);
#undef XX

unsigned int sleep(unsigned int seconds) {
  if (!sylar::t_hook_enable) {
    return sleep_f(seconds);
  }

  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();

 /*
  *	(void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) 是一个函数指针类型，
  *	它定义了一个指向 sylar::Scheduler 类中一个参数为 sylar::Fiber::ptr 和 int 类型的成员函数的指针类型。
  *	具体来说，它的含义如下：
  *	void 表示该成员函数的返回值类型，这里是 void 类型。
  *	(sylar::Scheduler::*) 表示这是一个 sylar::Scheduler 类的成员函数指针类型。
  *	(sylar::Fiber::ptr, int thread) 表示该成员函数的参数列表
  *       ，其中第一个参数为 sylar::Fiber::ptr 类型，第二个参数为 int 类型。
  *	
  *	使用 std::bind 绑定了 sylar::IOManager::schedule 函数，
  * 	并将 iom 实例作为第一个参数传递给了 std::bind 函数，将sylar::IOManager::schedule函数绑定到iom对象上。
  * 	在这里，第二个参数使用了函数指针类型 (void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread))
  * 	，表示要绑定的函数类型是 sylar::Scheduler 类中一个参数为 sylar::Fiber::ptr 和 int 类型的成员函数
  * 	，这样 std::bind 就可以根据这个函数类型来实例化出一个特定的函数对象，并将 fiber 和 -1 作为参数传递给它。
  */

  iom->addTimer(
      seconds * 1000,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));
  sylar::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (!sylar::t_hook_enable) {
    return usleep_f(usec);
  }
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();

  iom->addTimer(
      usec / 1000,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));
  sylar::Fiber::YieldToHold();
  return 0;
}

int nanosleep(const struct timespec* req, struct timespec* rem) {
  if (!sylar::t_hook_enable) {
    return nanosleep(req, rem);
  }
  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  sylar::Fiber::ptr fiber = sylar::Fiber::GetThis();
  sylar::IOManager* iom = sylar::IOManager::GetThis();
  iom->addTimer(
      timeout_ms,
      std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr, int thread)) &
                    sylar::IOManager::schedule,
                iom, fiber, -1));
  sylar::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (!sylar::t_hook_enable) {
    return socket_f(domain, type, protocol);
  }
  int fd = socket_f(domain, type, protocol);
  if (fd == -1) {
    return fd;
  }
  // 将fd放入到文件管理中
  sylar::FdMgr::GetInstance()->get(fd, true);
  return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen,
                         uint64_t timeout_ms) {
  if (!sylar::t_hook_enable) {
    return connect_f(fd, addr, addrlen);
  }
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if (!ctx || ctx->isClose()) {
    errno = EBADF;
    return -1;
  }
  if (!ctx->isSocket()) {
    return connect_f(fd, addr, addrlen);
  }
  if (ctx->getUserNonblock()) {
    return connect_f(fd, addr, addrlen);
  }

  // ----- 异步开始 -----
  // 先尝试连接
  int n = connect_f(fd, addr, addrlen);
  if (n == 0) {
    return 0;
  // 其他错误，EINPROGRESS表示连接操作正在进行中
  } else if (n != -1 || errno != EINPROGRESS) {
    return n;
  }
  sylar::IOManager* iom = sylar::IOManager::GetThis();
  sylar::Timer::ptr timer;
  std::shared_ptr<timer_info> tinfo(new timer_info);
  std::weak_ptr<timer_info> winfo(tinfo);

  // 设置了超时时间
  if (timeout_ms != (uint64_t)-1) {
    // 添加条件定时器
    timer = iom->addConditionTimer(
        timeout_ms,
        [winfo, fd, iom]() {
          auto t = winfo.lock();
          if (!t || t->cancelled) {
            return;
          }
          t->cancelled = ETIMEDOUT;
          iom->cancelEvent(fd, sylar::IOManager::WRITE);
        },
        winfo);
  }
  // 添加一个写事件
  int rt = iom->addEvent(fd, sylar::IOManager::WRITE);
  if (rt == 0) {
     /* 	只有两种情况唤醒：
      * 	1. 超时，从定时器唤醒
      *	  2. 连接成功，从epoll_wait拿到事件 */
    sylar::Fiber::YieldToHold();
    if (timer) {
      timer->cancel();
    }
    // 从定时器唤醒，超时失败
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {  // 添加事件失败
    if (timer) {
      timer->cancel();
    }
    SYLAR_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ",WRITE) errno";
  }
  int error = 0;
  socklen_t len = sizeof(int);
  // 获取套接字的错误状态
  if (-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) {
    return -1;
  }
  // 没有错误，连接成功
  if (!error) {
    return 0;
  // 有错误，连接失败
  } else {
    errno = error;
    return -1;
  }
}

int connect(int socket, const struct sockaddr* address, socklen_t address_len) {
  return connect_with_timeout(socket, address, address_len,
                              sylar::s_connect_timeout);
}

int accept(int socket, struct sockaddr* address, socklen_t* address_len) {
  int fd = do_io(socket, accept_f, "accept", sylar::IOManager::READ,
                 SO_RCVTIMEO, address, address_len);
  if (fd >= 0) {
    sylar::FdMgr::GetInstance()->get(fd, true);
  }
  return fd;
}

ssize_t read(int fd, void* buf, size_t count) {
  return do_io(fd, read_f, "read", sylar::IOManager::READ, SO_RCVTIMEO, buf,
               count);
}

ssize_t readv(int fildes, const struct iovec* iov, int iovcnt) {
  return do_io(fildes, readv_f, "readv", sylar::IOManager::READ, SO_RCVTIMEO,
               iov, iovcnt);
}

ssize_t recv(int socket, void* buffer, size_t length, int flags) {
  return do_io(socket, recv_f, "recv", sylar::IOManager::READ, SO_RCVTIMEO,
               buffer, length, flags);
}

ssize_t recvfrom(int socket, void* buffer, size_t length, int flags,
                 struct sockaddr* address, socklen_t* address_len) {
  return do_io(socket, recvfrom_f, "recvfrom", sylar::IOManager::READ,
               SO_RCVTIMEO, buffer, length, flags, address, address_len);
}

ssize_t recvmsg(int socket, struct msghdr* message, int flags) {
  return do_io(socket, recvmsg_f, "recvmsg", sylar::IOManager::READ,
               SO_RCVTIMEO, message, flags);
}

ssize_t write(int fd, const void* buf, size_t count) {
  return do_io(fd, write_f, "write", sylar::IOManager::WRITE, SO_SNDTIMEO, buf,
               count);
}

ssize_t writev(int fd, const struct iovec* iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", sylar::IOManager::WRITE, SO_SNDTIMEO,
               iov, iovcnt);
}

ssize_t send(int socket, const void* buffer, size_t length, int flags) {
  return do_io(socket, send_f, "send", sylar::IOManager::WRITE, SO_SNDTIMEO,
               buffer, length, flags);
}

ssize_t sendto(int socket, const void* message, size_t length, int flags,
               const struct sockaddr* dest_addr, socklen_t dest_len) {
  return do_io(socket, sendto_f, "send_to", sylar::IOManager::WRITE,
               SO_SNDTIMEO, message, length, flags, dest_addr, dest_len);
}

ssize_t sendmsg(int socket, const struct msghdr* message, int flags) {
  return do_io(socket, sendmsg_f, "sendmsg", sylar::IOManager::WRITE,
               SO_SNDTIMEO, message, flags);
}

int close(int fd) {
  if (!sylar::t_hook_enable) {
    return close(fd);
  }
  sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
  if (ctx) {
    auto iom = sylar::IOManager::GetThis();
    if (iom) {
      iom->cancelAll(fd);
    }
    sylar::FdMgr::GetInstance()->del(fd);
  }
  return close_f(fd);
}

/**
 * 对用户反馈是否是用户设置了非阻塞模式。
 */
int fcntl(int fildes, int cmd, ...) {
  va_list va;
  va_start(va, cmd);
  switch (cmd) {
    case F_SETFL: {
      int arg = va_arg(va, int);
      va_end(va);
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fildes);
      if (!ctx || ctx->isClose() || ctx->isSocket()) {
        return fcntl_f(fildes, cmd, arg);
      }
      ctx->setUserNonblock(arg & O_NONBLOCK);
      if (ctx->getSysNonblock()) {
        arg |= O_NONBLOCK;
      } else {
        arg &= ~O_NONBLOCK;
      }
      return fcntl_f(fildes, cmd, arg);
    } break;
    case F_GETFL: {
      va_end(va);
      int arg = fcntl(fildes, cmd);
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fildes);
      if (!ctx || ctx->isClose() || !ctx->isSocket()) {
        return arg;
      }
      if (ctx->getUserNonblock()) {
        return arg | O_NONBLOCK;
      } else {
        return arg & ~O_NONBLOCK;
      }
    } break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
    case F_SETSIG:
    case F_SETLEASE:
    case F_NOTIFY:
    case F_SETPIPE_SZ: {
      int arg = va_arg(va, int);
      va_end(va);
      return fcntl_f(fildes, cmd, arg);
    } break;
    case F_GETFD:
    case F_GETOWN:
    case F_GETSIG:
    case F_GETLEASE:
    case F_GETPIPE_SZ: {
      va_end(va);
      return fcntl_f(fildes, cmd);
    } break;
    case F_SETLK:
    case F_SETLKW:
    case F_GETLK: {
      struct flock* arg = va_arg(va, struct flock*);
      va_end(va);
      return fcntl_f(fildes, cmd, arg);
    } break;
    case F_GETOWN_EX:
    case F_SETOWN_EX: {
      struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
      va_end(va);
      return fcntl_f(fildes, cmd, arg);
    } break;
    default:
      va_end(va);
      return fcntl(fildes, cmd);
  }
}

/**
 * 对设备进行控制操作
 */
/*	value为指向int类型的指针，如果该指针指向的值为0，则表示关闭非阻塞模式；如果该指针指向的值为非0，则表示打开非阻塞模式。
 *	int value = 1;
 *	ioctl(fd, FIONBIO, &value);
 */
int ioctl(int fildes, unsigned long int intrequest, ...) {
  va_list va;
  va_start(va, intrequest);
  void* arg = va_arg(va, void*);
  va_end(va);
  if (FIONBIO == intrequest) {
    bool user_nonblock = !!*(int*)arg;
    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fildes);
    if (!ctx || ctx->isSocket() || ctx->isClose()) {
      return ioctl(fildes, intrequest, arg);
    }
    ctx->setUserNonblock(user_nonblock);
  }
  return ioctl_f(fildes, intrequest, arg);
}

int getsockopt(int socket, int level, int option_name, void* option_value,
               socklen_t* option_len) {
  return getsockopt_f(socket, level, option_name, option_value, option_len);
}

/**
 * 设置socket选项
 */
int setsockopt(int socket, int level, int option_name, const void* option_value,
               socklen_t option_len) {
  if (!sylar::t_hook_enable) {
    return setsockopt_f(socket, level, option_name, option_value, option_len);
  }
  // 如果设置socket通用选项
  if (level == SOL_SOCKET) {
    // 如果设置超时选项
    if (option_name == SO_RCVTIMEO || option_name == SO_SNDTIMEO) {
      sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(socket);
      if (ctx) {
        const timeval* v = (const timeval*)option_value;
        // 转为毫秒保存
        ctx->setTimeout(option_name, v->tv_sec * 1000 + v->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(socket, level, option_name, option_value, option_len);
}
}
