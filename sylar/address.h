#ifndef __SYLAR_ADDRESS_H__
#define __SYLAR_ADDRESS_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "endian.h"

namespace sylar {
class IPAddress;
/**
 * @brief 网络地址的基类抽象类
 *
 * */
class Address {
 public:
  typedef std::shared_ptr<Address> ptr;
  /**
   * @brief 通过sockaddr指针创建Address
   * @param[in] addr sockaddr指针
   * @param[in] addrlen sockaddr的长度
   * @return 返回和sockaddr相匹配的Address，失败返回nullptr
   * */
  static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

  /**
   * @brief 通过host地址返回对应条件的所有Address
   * @param [out] result 保存满足条件的Address
   * @param [in] host 域名，服务器名，举例子：www.baidu.com[:80](方括号内为可选内容)
   * @param [in] family 协议族
   * @param [in] type socket类型SOCK_STREAM,SOCK_DGRAM等
   * @return 返回是否转换成功
   * */
  static bool Lookup(std::vector<Address::ptr>& result, const std::string& host,
                     int family = AF_INET, int type = 0, int protocol = 0);

  /**
   * @brief 通过host地址返回对应条件的任意Address
   * @param [in] host 域名，服务器名，举例子：www.baidu.com[:80](方括号内为可选内容)
   * @param [in] family 协议族
   * @param [in] type socket类型SOCK_STREAM,SOCK_DGRAM等
   * @return 返回满足条件的任意的Address，失败返回nullptr
   * */
  static Address::ptr LookupAny(const std::string& host, int family = AF_INET,
                                int type = 0, int protocol = 0);

  /**
   * @brief 通过host地址返回对应条件的任意Address
   * @param [in] host 域名，服务器名，举例子：www.baidu.com[:80](方括号内为可选内容)
   * @param [in] family 协议族
   * @param [in] type socket类型SOCK_STREAM,SOCK_DGRAM等
   * @return 返回满足条件的任意IPAddress，失败返回nullptr
   * */
  static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
                                                       int family = AF_INET,
                                                       int type = 0,
                                                       int protocol = 0);

  /**
   * @brief 返回本机所有网卡的<网卡名，地址，子网掩码位数>
   * @param[out] result 保存本机所有地址
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @return 是否获取成功
   * */
  static bool GetInterfaceAddresses(
      std::multimap<std::string, std::pair<Address::ptr, uint32_t>>& result,
      int family = AF_INET);

  /**
   * @brief 返回本机所有网卡的<网卡名，地址，子网掩码位数>
   * @param[out] result 保存本机所有地址
   * @param[in] family 协议族(AF_INT, AF_INT6, AF_UNIX)
   * @return 是否获取成功
   * */
  static bool GetInterfaceAddresses(
      std::vector<std::pair<Address::ptr, uint32_t>>& result,
      const std::string& iface, int family = AF_INET);

  /**
   * @brief 析构函数
   * */
  virtual ~Address() {}

  /**
   * @brief 返回协议族
   * */
  int getFamily() const;

  /**
   * @brief 返回sockaddr指针，读写
   * */
  virtual const sockaddr* getAddr() const = 0;

  /**
   * @brief 返回sockaddr的长度
   * */
  virtual socklen_t getAddrLen() const = 0;

  /**
   * @brief 可读性输出地址
   * */
  virtual std::ostream& insert(std::ostream& os) const = 0;

  /**
   * @brief 返回可读性字符串
   * */
  std::string toString() const;

  bool operator<(const Address& rhs) const;
  bool operator==(const Address& rhs) const;
  bool operator!=(const Address& rhs) const;
};

class IPAddress : public Address {
 public:
  typedef std::shared_ptr<IPAddress> ptr;

  static IPAddress::ptr Create(const char* address, uint16_t port = 0);

  /**
   * @brief 获取该地址的广播地址
   * @param[in] prefix_len 子网掩码位数
   * @return 调用成功返回IPAddress，失败返回nullptr
   * */
  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

  /**
   * @brief 获取该地址的网段
   * @param[in] prefix_len 子网掩码位数
   * @return 调用成功返回IPAddress，失败返回nullptr
   * */
  virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

  /**
   * @brief 获取子网掩码地址
   * @param[in] prefix_len 子网掩码位数
   * @return 调用成功返回IPAddress，失败返回nullptr
   * */
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  /**
   * @brief 返回端口号
   * */
  virtual uint32_t getPort() const = 0;

  /**
   *@brief 设置端口号
   * */
  virtual void setPort(uint16_t v) = 0;
};

/**
 * @brief IPv4地址
 * */
class IPv4Address : public IPAddress {
 public:
  typedef std::shared_ptr<IPv4Address> ptr;

  static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

  /**
   * @brief 通过二进制地址构造IPv4Address
   * @param[in] address 二进制地址address
   * @param[in] port 端口号
   * */
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  IPv4Address(const sockaddr_in& address);

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint32_t getPort() const override;
  void setPort(uint16_t v) override;

 private:
  sockaddr_in m_addr;
};

/**
 * @brief IPv6地址
 * */
class IPv6Address : public IPAddress {
 public:
  typedef std::shared_ptr<IPv6Address> ptr;

  static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

  IPv6Address(const uint8_t address[16], uint16_t port = 0);

  IPv6Address();
  IPv6Address(const sockaddr_in6& address);

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;

  uint32_t getPort() const override;
  void setPort(uint16_t v) override;

 private:
  sockaddr_in6 m_addr;
};

/**
 * @brief Unixsocket地址
 * */
class UnixAddress : public Address {
 public:
  std::shared_ptr<UnixAddress> ptr;
  UnixAddress();
  UnixAddress(const std::string& path);

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

 private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

/**
 * @brief 未知的地址
 * */
class UnknowAddress : public Address {
 public:
  typedef std::shared_ptr<UnknowAddress> ptr;

  UnknowAddress(int family);
  UnknowAddress(const sockaddr& addr);

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert(std::ostream& os) const override;

 private:
  sockaddr m_addr;
};

}  // namespace sylar

#endif