#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include "address.h"
#include "hook.h"
#include "noncopyable.h"

namespace sylar {

/**
 * @brief Socket封装类
 * */
class Socket : public std::enable_shared_from_this<Socket>, Noncopyable {
 public:
  typedef std::shared_ptr<Socket> ptr;
  typedef std::weak_ptr<Socket> weak_ptr;

  /**
   * @brief socket 类型
   * */
  enum Type {
    /// TCP 类型
    TCP = SOCK_STREAM,
    /// UDP 类型
    UDP = SOCK_DGRAM
  };

  /**
   * @brief socket协议族
   * */
  enum Family {
    /// IPv4 socket
    IPv4 = AF_INET,
    /// IPv6 socket
    IPv6 = AF_INET6,
    /// Unix socket
    UNIX = AF_UNIX,
  };

  /**
   * @brief 创建TCP Socket
   * @param [in] address 地址
   * */
  static Socket::ptr CreateTCP(sylar::Address::ptr address);

  /**
   * @brief 创建UDP Socket满足地址类型
   * @param [in] address 地址
   * */
  static Socket::ptr CreateUDP(sylar::Address::ptr address);

  static Socket::ptr CreateTCPSocket();
  static Socket::ptr CreateUDPSocket();

  static Socket::ptr CreateTCPSocket6();
  static Socket::ptr CreateUDPSocket6();

  static Socket::ptr CreateUnixTCPSocket();
  static Socket::ptr CreateUnixUDPSocket();

  /**
   * @brief Socket构造函数
   * @param[in] family 协议族
   * @param[in] type 类型
   * @param[in] protocol 协议
   * */
  Socket(int family, int type, int protocol = 0);

  /**
   * @brief 析构函数
   * */
  ~Socket();

  /**
   * @brief 获取发送超时时间
   * */
  int64_t getSendTimeout();

  /**
    *@brief 设置发送超时时间
    * @param v 设置
    * */
  void setSendTimeout(uint64_t v);

  /**
    * @brief 获取接受超时时间
    * */
  int64_t getRecvTimeout();

  /**
    * @brief 设置接受超时时间
    * */
  void setRecvTimeout(uint64_t v);

  /**
    *@brief获取 sockopt @see getsockopt
    * */
  bool getOption(int level, int option, void* result, socklen_t* len);

  /**
     *@brief 获取sockopt
     * */
  template <class T>
  bool getOption(int level, int option, T& result) {
    socklen_t length = sizeof(T);
    return getOption(level, option, result, &length);
  }

  /**
     * @brief 设置sockopt @see setsockopt
     * */
  bool setOption(int level, int option, const void* result, socklen_t len);

  /**
     *@brief 获取sockopt
     * */
  template <class T>
  bool setOption(int level, int option, const T& value) {
    return setOption(level, option, &value, sizeof(T));
  }

  /**
     *@brief 接受connect连接
     * @return 成功返回新连接的socket，失败返回nullptr
     * @pre Socket必须bind，listen 成功
     * */
  virtual Socket::ptr accept();

  /**
      * @brief 绑定地址
      * @param[in] addr 地址
      * @return 是否绑定成功
      * */
  virtual bool bind(const Address::ptr addr);

  /**
      * @brief 连接地址
      * @param[in] addr目标地址
      * @param[in] timeout_ms 超时时间ms
      * */
  virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);

  /**
       * @brief
       * */
  virtual bool reconnect(uint64_t timeout_ms = -1);

  /**
       * @brief 监听socket
       * @param[in] backlog 未完成连接队列的最大长度
       * @result  返回监听是否成功
       * @pre 必须先bind成功
       * */
  virtual bool listen(int backlog = SOMAXCONN);

  virtual bool close();

  /**
       * @brief 发送数据
       * @param[in] buffer 待发送的数据的内存
       * @param[in] length 待发送的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 发送成功对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int send(const void* buffer, size_t length, int flags = 0);

  /**
       * @brief 发送数据
       * @param[in] buffer 待发送的数据的内存
       * @param[in] length 待发送的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 发送成功对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int send(const iovec* buffers, size_t length, int flags = 0);

  /**
       * @brief 发送数据
       * @param[in] buffer 待发送的数据的内存
       * @param[in] length 待发送的数据长度
       * @param[in] to  发送的地址
       * @param[in] flags 标志字
       * @return
       *    @retval >0 发送成功对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int sendTo(const void* buffer, size_t length, const Address::ptr to,
                     int flags = 0);

  /**
       * @brief 发送数据
       * @param[in] buffer 待发送的数据的内存
       * @param[in] length 待发送的数据长度
       * @param[in] to  发送的地址
       * @param[in] flags 标志字
       * @return
       *    @retval >0 发送成功对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to,
                     int flags = 0);

  /**
       * @brief 接收数据
       * @param[in] buffer 待发送的数据的内存
       * @param[in] length 待发送的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 接收到对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int recv(void* buffer, size_t length, int flags = 0);

  /**
       * @brief 接收数据
       * @param[in] buffer 待接收的数据的内存
       * @param[in] length 待接收的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 接收到对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int recv(iovec* buffers, size_t length, int flags = 0);

  /**
       * @brief 接收数据
       * @param[in] buffer 待接收的数据的内存
       * @param[in] from 发送端地址
       * @param[in] length 待接收的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 接收到对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int recvFrom(void* buffers, size_t length, Address::ptr from,
                       int flags = 0);

  /**
       * @brief 接收数据
       * @param[in] buffer 待接收的数据的内存
       * @param[in] from   发送端地址
       * @param[in] length 待接收的数据长度
       * @param[in] flags 标志字
       * @return
       *    @retval >0 接收到对应大小的数据
       *    @retval = 0 socket被关闭
       *    @retval < 0 socket出错
       * */
  virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from,
                       int flags = 0);

  /**
       * @brief 获取远端地址
       * */
  Address::ptr getRemoteAddress();

  /**
       * @brief 获取本地地址
       * */
  Address::ptr getLocalAddress();

  int getFamily() const { return m_family; }
  int getType() const { return m_type; }
  int getProtocol() const { return m_protocol; }
  bool isConnected() const { return m_isConnect; }

  /**
   * @brief 返回是否有效
   * */
  bool isValid() const;

  /**
   * @brief 返回socket错误
   * */
  int getError();

  /**
   * @brief 输出socket信息到流中
   * */
  virtual std::ostream& dump(std::ostream& os) const;
  virtual std::string toString() const;

  int getSocket() const { return m_sock; }

  /**
       * @brief 撤销读
       * */
  bool cancelRead();

  /**
       * @brief 撤销写
       * */
  bool cancelWrite();

  /**
       * @brief撤销accept
       * */
  bool cancelAccept();

  /**
        * @brief 撤销所有事件
        * */
  bool cancelAll();

 protected:
  /**
   * @brief 初始化socket
   * */
  void initSock();

  /**
   * @brief 创建socket
   * */
  void newSock();

  /**
   * @brief 初始化socket
   * */
  virtual bool init(int sock);

 private:
  /// socket句柄
  int m_sock;
  /// 协议族
  int m_family;
  /// 类型
  int m_type;
  /// 协议
  int m_protocol;
  /// 是否连接
  bool m_isConnect;
  /// 本地地址
  Address::ptr m_localAddress;
  /// 远程地址
  Address::ptr m_remoteAddress;
};

std::ostream& operator<<(std::ostream& os, const Socket& sock);

}  // namespace sylar

#endif