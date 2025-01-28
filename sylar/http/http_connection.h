#ifndef __SYLAR_HTTP_CONNECTION_H__
#define __SYLAR_HTTP_CONNECTION_H__

#include <list>
#include "http.h"
#include "sylar/streams/socket_stream.h"
#include "sylar/thread.h"
#include "sylar/uri.h"

namespace sylar {

namespace http {

/**
 * @brief HTTP响应结果
 */

struct HttpResult {
  /// 智能指针类型定义
  typedef std::shared_ptr<HttpResult> ptr;

  /**
   * @brief 错误码定义
   */
  enum class Error {
    /// 正常
    OK = 0,
    /// 非法URL
    INVALID_URL = 1,
    /// 无法解析HOST
    INVALID_HOST = 2,
    /// 连接失败
    CONNECT_FAIL = 3,
    /// 连接被对端关闭
    SEND_CLOSE_BY_PEER = 4,
    /// 发送请求产生Socket错误
    SEND_SOCKET_ERROR = 5,
    /// 超时
    TIMEOUT = 6,
    /// 创建Socket失败
    CREATE_SOCKET_ERROR = 7,
    /// 从连接池中取连接失败
    POOL_GET_CONNECTION = 8,
    /// 无效的连接
    POOL_INVALID_CONNECTION = 9,
  };

  /**
   * @brief 构造函数
   * @param _result 错误码
   * @param _response HTTP响应结构体
   * @param _error  错误概述
   */
  HttpResult(int _result, HttpResponse::ptr _response,
             const std::string& _error)
      : result(_result), response(_response), error(_error) {}

  /// 错误码
  int result;
  /// HTTP响应结构体
  HttpResponse::ptr response;
  /// 错误描述
  std::string error;

  std::string toString() const;
};

class HttpConnectionPool;

/**
 * @brief HttpConnection封装
 */
class HttpConnection : public SocketStream {
  friend class HttpConnectionPool;

 public:
  /// 智能指针类型定义
  typedef std::shared_ptr<HttpConnection> ptr;

  /**
   * @brief 发送HTTP的GET请求
   * @param url 请求url
   * @param timeout_ms 超时时间
   * @param headers headers HTTP请求头部参数
   * @param body body请求消息体
   * @return 返回HTTP结果结构体
   */
  static HttpResult::ptr DoGet(
      const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoGet(
      Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP的POST请求
     * @param[in] url 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoPost(
      const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoPost(
      Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoRequest(
      HttpMethod method, const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoRequest(
      HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] req 请求结构体
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return 返回HTTP结果结构体
     */
  static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri,
                                   uint64_t timeout_ms);

  /***
   * @brief 构造函数
   * @param sock socket类型
   * @param owner 是否托管
   */
  HttpConnection(Socket::ptr sock, bool owner = true);

  /**
   * @brief 析构函数
   */
  ~HttpConnection();

  /**
   * @brief 接收HTTP请求
   */
  HttpResponse::ptr recvResponse();

  int sendRequest(HttpRequest::ptr req);

 private:
  uint64_t m_createTime = 0;
  uint64_t m_request = 0;
};

/**
 * @brief 连接池
 */
class HttpConnectionPool {

 public:
  // 智能指针
  typedef std::shared_ptr<HttpConnectionPool> ptr;
  // 互斥锁
  typedef Mutex MutexType;

  static HttpConnectionPool::ptr Create(const std::string& uri,
                                        const std::string& vhost,
                                        uint32_t max_size,
                                        uint32_t max_alive_time,
                                        uint32_t max_request);

  /**
   * @brief 构造函数
   */
  HttpConnectionPool(const std::string& host, const std::string& vhost,
                     uint32_t port, uint32_t max_size, uint32_t max_alive_time,
                     uint32_t max_request);
  /**
   * @brief 取连接
   * @return
   */
  HttpConnection::ptr getConnection();

  /**
     * @brief 发送HTTP的GET请求
     * @param[in] url 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doGet(const std::string& url, uint64_t timeout_ms,
                        const std::map<std::string, std::string>& headers = {},
                        const std::string& body = "");

  /**
     * @brief 发送HTTP的GET请求
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms,
                        const std::map<std::string, std::string>& headers = {},
                        const std::string& body = "");

  /**
     * @brief 发送HTTP的POST请求
     * @param[in] url 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doPost(const std::string& url, uint64_t timeout_ms,
                         const std::map<std::string, std::string>& headers = {},
                         const std::string& body = "");

  /**
     * @brief 发送HTTP的POST请求
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms,
                         const std::map<std::string, std::string>& headers = {},
                         const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri 请求的url
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doRequest(
      HttpMethod method, const std::string& url, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] method 请求类型
     * @param[in] uri URI结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @param[in] headers HTTP请求头部参数
     * @param[in] body 请求消息体
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doRequest(
      HttpMethod method, Uri::ptr uri, uint64_t timeout_ms,
      const std::map<std::string, std::string>& headers = {},
      const std::string& body = "");

  /**
     * @brief 发送HTTP请求
     * @param[in] req 请求结构体
     * @param[in] timeout_ms 超时时间(毫秒)
     * @return 返回HTTP结果结构体
     */
  HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);

 private:
  static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);

 private:
  std::string m_host;
  std::string m_vhost;
  uint32_t m_port;
  // 连接池的最多的连接个数
  uint32_t m_maxSize;
  // 最大的存在时间，如果超出这个时间没有连接上就关闭
  uint32_t m_maxAliveTime;
  // 每个连接的最大请求个数
  uint32_t m_maxRequest;
  // 锁
  MutexType m_mutex;
  std::list<HttpConnection*> m_conns;
  // 计数
  std::atomic<int32_t> m_total = {0};
};

}  // namespace http
}  // namespace sylar

#endif