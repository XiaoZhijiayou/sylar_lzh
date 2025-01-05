#ifndef __SYLAR_SOCKET_STREAM_H__
#define __SYLAR_SOCKET_STREAM_H__

#include "sylar/stream.h"
#include "sylar/socket.h"
#include "sylar/mutex.h"
#include "sylar/iomanager.h"


namespace sylar{

/**
 * @brief Sock流
 */
class SocketStream : public Stream{
 public:
  typedef std::shared_ptr<SocketStream> ptr;

  SocketStream(Socket::ptr sock, bool owner = true);

  ~SocketStream();

  virtual int read (void* buffer, size_t length) override;

  virtual int read(ByteArray::ptr ba, size_t length) override;

  virtual int write(const void* buffer, size_t length) override;

  virtual int write(ByteArray::ptr ba, size_t length) override;

  virtual void close() override;

  Socket::ptr getSocket() const {return m_socket;}

  bool isConnected() const;

  Address::ptr getRemoteAddress();
  Address::ptr getLocalAddress();
  std::string getRemoteAddressString();
  std::string getLocalAddressString();
 protected:
  /// socket类
  Socket::ptr m_socket;
  /// 是否主控
  bool m_owner;

};

}

#endif