#ifndef __SYLAR__HTTP_PARSER_H__
#define __SYLAR__HTTP_PARSER_H__

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace sylar{
namespace http{

/**
 * @brief HTTP请求解析类
 * */
class HttpRequestParser{
 public:
  /// HTTP解析类的智能指针
  typedef std::shared_ptr<HttpRequestParser> ptr;
  /**
     * @brief 构造函数
     */
  HttpRequestParser();

  /**
     * @brief 解析协议
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @return 返回实际解析的长度,并且将已解析的数据移除
     */
  size_t execute(char* data, size_t len);

  /**
   * @brief 是否解析完成
   * @return 是否解析完成
   * */
  bool isFinished();

  /**
   *@brief 是否有错误
   * @return 是否有错误
   * */
  int hasError();

  /**
   * @brief 返回HttpRequest结构体
   * */
  HttpRequest::ptr getData() const {return m_data;}

  /**
   * @brief 设置错误
   * @param[in] v 错误信息
   * */
   void setError(int v){m_error = v;}

   /**
    * @brief 获取消息体长度
    * */
    uint64_t getContentLenght();

    /**
     * @brief 获取http_parser结构体
     * */
    const http_parser& getParser() const {return m_parser;}

 protected:

  /**
   * @brief 返回HttpRequest协议解析的缓存大小
   * */
  static uint64_t GetHttpRequestBufferSize();

  /**
   * @brief 返回HttpRequest协议解析的最大消息体大小
   * */
  static uint64_t GetHttpRequestMaxBodySize();
 private:
  /// http_parser
  http_parser m_parser;
  /// HttpRequest结构
  HttpRequest::ptr m_data;

  /// 错误码
  /// 1000: invalid method
  /// 1001: invalid version
  /// 1002: invalid field
  int m_error;
};


/**
 * @brief HTTP响应解析类
 * */
class HttpResponseParser{
 public:
  /// 智能指针类型
  typedef std::shared_ptr<HttpResponseParser> ptr;

  /**
   * @brief 构造函数
   * */
   HttpResponseParser();

   /**
     * @brief 解析HTTP响应协议
     * @param[in, out] data 协议文本内存
     * @param[in] len 协议文本内存长度
     * @param [in] chunck 是否在解析chunck
     * @return 返回实际解析的长度,并且将已解析的数据移除
     */
   size_t execute(char* data, size_t len, bool chunck);

   /**
   * @brief 是否解析完成
   * @return 是否解析完成
   * */
   bool isFinished();

   /**
   *@brief 是否有错误
   * @return 是否有错误
   * */
   int hasError();

   /**
   * @brief 返回HttpResponse结构体
   * */
   HttpResponse::ptr getData() const {return m_data;}

   /**
   * @brief 设置错误
   * @param[in] v 错误信息
   * */
   void setError(int v){m_error = v;}

   /**
    * @brief 获取消息体长度
    * */
   uint64_t getContentLenght();

   /**
     * @brief 获取httpclient_parser结构体
     * */
   const httpclient_parser& getParser() const {return m_parser;}

  protected:

   /**
   * @brief 返回HttpResponse协议解析的缓存大小
   * */
   static uint64_t GetHttpResponseBufferSize();

   /**
   * @brief 返回HttpResponse协议解析的最大消息体大小
   * */
   static uint64_t GetHttpResponseMaxBodySize();
 private:
  /// httpcilent_parser
  httpclient_parser m_parser;
  /// HttpResponse
  HttpResponse::ptr m_data;
  /// 错误码
  /// 1001: invalid version
  /// 1002: invalid field
  int m_error;
};



}
}

#endif