#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "sylar/tcp_server.h"
#include "sylar/http/http_session.h"

namespace sylar{
namespace http{

/**
 * @brief HttpServer 服务类
 */
class HttpServer : public TcpServer{
 public:
  typedef std::shared_ptr<HttpServer> ptr;

  /**
   * @brief 构造函数
   * @param keepalive 是否长连接
   * @param worker 工作调度器
   * @param accept_worker 接收连接器
   */
  HttpServer(bool keepalive = false
             ,sylar::IOManager* worker = sylar::IOManager::GetThis()
             ,sylar::IOManager* accept_worker = sylar::IOManager::GetThis());



 protected:
  virtual void handleClient(Socket::ptr client) override;

 private:

  /// 是否支持长连接
  bool m_isKeepalive;

};

}
}

#endif