#ifndef __SYLAR_HTTP_SERVER_H__
#define __SYLAR_HTTP_SERVER_H__

#include "servlet.h"
#include "sylar/http/http_session.h"
#include "sylar/tcp_server.h"

namespace sylar {
namespace http {

/**
 * @brief HttpServer 服务类
 */
class HttpServer : public TcpServer {
 public:
  typedef std::shared_ptr<HttpServer> ptr;

  /**
   * @brief 构造函数
   * @param keepalive 是否长连接
   * @param worker 工作调度器
   * @param accept_worker 接收连接器
   */
  HttpServer(bool keepalive = false,
             sylar::IOManager* worker = sylar::IOManager::GetThis(),
             sylar::IOManager* accept_worker = sylar::IOManager::GetThis());

  /**
   * @brief 获取ServletDispatch
   */
  ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }

  /**
   * @brief 设置ServletDispatch
   * @param v
   */
  void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }

 protected:
  virtual void handleClient(Socket::ptr client) override;

 private:
  /// 是否支持长连接
  bool m_isKeepalive;
  /// Servlet分发器
  ServletDispatch::ptr m_dispatch;
};

}  // namespace http
}  // namespace sylar

#endif