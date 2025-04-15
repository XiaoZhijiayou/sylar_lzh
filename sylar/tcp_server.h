/**
 * @brief TCP服务器的封装
 */

#ifndef __SYLAR_TCP_SERVER_H__
#define __SYLAR_TCP_SERVER_H__

#include <functional>
#include <memory>
#include "address.h"
#include "config.h"
#include "iomanager.h"
#include "noncopyable.h"
#include "socket.h"

namespace sylar {

/**
 * @brief TCP服务器封装
 */
class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable {
 public:
  typedef std::shared_ptr<TcpServer> ptr;

  /**
   * @brief 构造函数
   * @param name 服务器名称
   * @param type 服务器类型
   * @param worker socket客户端工作的协程调度器
   * @param accept_worker 服务器socket执行接收socket连接的协程调度器
   */
  TcpServer(sylar::IOManager* worker = sylar::IOManager::GetThis(),
            sylar::IOManager* io_worker = sylar::IOManager::GetThis(),  
            sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

  /**
   * @brief 析构函数
   */
  virtual ~TcpServer();

  /**
   * @brief 绑定地址
   * @param addr 传入绑定地址
   * @return 返回是否绑定成功
   */
  virtual bool bind(sylar::Address::ptr addr);

  /**
   * @brief 绑定地址数组
   * @param addr 需要绑定的地址数组
   * @param fails 绑定失败的地址
   * @return 是否绑定成功
   */
  virtual bool bind(const std::vector<Address::ptr>& addrs,
                    std::vector<Address::ptr>& fails);

  /**
   * @brief 启动服务
   * @return 需要bind成功之后执行
   */
  virtual bool start();

  /**
   * @brief 停止服务
   */
  virtual void stop();

  uint64_t getRecvTimeout() const { return m_recvTimeout; }

  std::string getName() const { return m_name; }

  void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }

  virtual void setName(const std::string& v) { m_name = v; }

  bool isStop() const { return m_isStop; }

  virtual std::string tostring(const std::string& prefix = "");

 protected:
  /**
   * @brief 处理新连接的Socket类
   * @param client
   */
  virtual void handleClient(Socket::ptr client);

  /**
   * @brief 开始接受连接
   * @param sock
   */
  virtual void startAccept(Socket::ptr sock);

 protected: // 之前是private，现在为了迎合rpc部分的功能，改为protected
  /// 监听socket数组
  std::vector<Socket::ptr> m_socks;
  /// 新连接的Socket工作的调度器
  IOManager* m_worker;
  IOManager* m_ioWorker;
  /// 服务器Socket接受连接的调度器
  IOManager* m_acceptWorker;
  /// 接收超时时间(毫秒)
  uint64_t m_recvTimeout;
  /// 服务器名称
  std::string m_name;
  /// 服务器类型
  std::string m_type = "tcp";
  /// 服务是否停止
  bool m_isStop;
};

}  // namespace sylar

#endif