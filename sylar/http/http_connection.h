#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include "sylar/streams/socket_stream.h"
#include "http.h"

namespace sylar{

namespace http{

/**
 * @brief HTTP响应结果
 */

//struct HttpResult{
//  /// 智能指针类型定义
//  typedef std::shared_ptr<>
//};

/**
 * @brief HttpConnection封装
 */
class HttpConnection : public SocketStream{
 public:
  /// 智能指针类型定义
  typedef std::shared_ptr<HttpConnection> ptr;

  /***
   * @brief 构造函数
   * @param sock socket类型
   * @param owner 是否托管
   */
  HttpConnection(Socket::ptr sock, bool owner = true);

  /**
   * @brief 接收HTTP请求
   */
  HttpResponse::ptr recvResponse();

  int sendRequset(HttpRequest::ptr req);
};

}

}


#endif