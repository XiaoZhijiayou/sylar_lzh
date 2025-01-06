#ifndef __SYLAR_HTTP_SESSION_H__
#define __SYLAR_HTTP_SESSION_H__

#include "sylar/streams/socket_stream.h"
#include "http.h"

namespace sylar{

namespace http{


/**
 * @brief HttpSession封装
 */
class HttpSession : public SocketStream{
 public:
  /// 智能指针类型定义
  typedef std::shared_ptr<HttpSession> ptr;

  /***
   * @brief 构造函数
   * @param sock socket类型
   * @param owner 是否托管
   */
  HttpSession(Socket::ptr sock, bool owner = true);

  /**
   * @brief 接收HTTP请求
   */
  HttpRequest::ptr recvRequest();

  int sendResponse(HttpResponse::ptr rsp);
};

}

}


#endif