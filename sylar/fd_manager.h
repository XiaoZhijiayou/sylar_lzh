#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include <vector>
#include "mutex.h"
#include "singleton.h"
#include "thread.h"

namespace sylar {

class FdCtx : public std::enable_shared_from_this<FdCtx> {
 public:
  typedef std::shared_ptr<FdCtx> ptr;
  FdCtx(int fd);
  ~FdCtx();

  bool init();

  bool isInit() const { return m_isInit; }

  bool isSocket() const { return m_isSocket; }

  bool isClose() const { return m_isClsed; }

  bool close();

  void setUserNonblock(bool v) { m_userNonblock = v; }

  bool getUserNonblock() const { return m_userNonblock; }

  void setSysNonblock(bool v) { m_sysNonblock = v; }

  bool getSysNonblock() const { return m_sysNonblock; }

  /**
   * @brief 设置fd的读写超时时间
   */
  void setTimeout(int type, uint64_t v);

  /**
   * @brief 获取fd的读写超时时间
   */
  uint64_t getTimeout(int type);

 private:
  /// 是否初始化
  bool m_isInit : 1;
  /// 是否socket
  bool m_isSocket : 1;
  /// 是否hook非阻塞
  bool m_sysNonblock : 1;
  /// 是否用户主动设置非阻塞
  bool m_userNonblock : 1;
  /// 是否关闭
  bool m_isClsed : 1;
  /// 文件句柄
  int m_fd;
  /// 读超时时间毫秒
  uint64_t m_recvTimeout;
  /// 写超时时间毫秒
  uint64_t m_sendTimeout;
};

class FdManager {
 public:
  typedef RWMutex RWMutexType;

  FdManager();

  FdCtx::ptr get(int fd, bool auto_create = false);
  
  /**
   * @brief 删除fd的上下文
   */
  void del(int fd);

 private:
  RWMutexType m_mutex;  /// 读写锁
  std::vector<FdCtx::ptr> m_datas;  /// 文件句柄集合
};

typedef Singleton<FdManager> FdMgr;
}  // namespace sylar

#endif