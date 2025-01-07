#ifndef __SYLAR_SERVLET_H__
#define __SYLAR_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include "http_session.h"
#include "sylar/mutex.h"

namespace sylar{
namespace http{

/**
 * @brief Servlet的封装
 */
class Servlet{
 public:
  /// 智能指针
  typedef std::shared_ptr<Servlet> ptr;

  /**
   * @brief 构造函数
   * @param name 名称
   */
  Servlet(const std::string& name)
      :m_name(name) { }

  /**
   * @brief 析构函数
   */
  virtual ~Servlet() {};

  /**
   * @brief 处理请求
   * @param request HTTP请求
   * @param response HTTP响应
   * @param session HTTP连接
   * @return    是否处理成功
   */
  virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         ,sylar::http::HttpResponse::ptr response
                         ,sylar::http::HttpSession::ptr session) = 0;

  /**
   * @brief 返回servlet的名称
   */
  const std::string& getName() const {return m_name;}

 protected:
  /// 名称
  std::string m_name;
};

class FunctionServlet : public Servlet{
 public:
  /// 智能指针类型的定义
  typedef std::shared_ptr<FunctionServlet> ptr;

  /// 函数回调类型的定义
  typedef std::function<int32_t (sylar::http::HttpRequest::ptr request
                                ,sylar::http::HttpResponse::ptr response
                                ,sylar::http::HttpSession::ptr session)> callback;
  /**
   * @brief 构造函数
   * @param cb 回调函数
   */
  FunctionServlet(callback cb);
  virtual  int32_t handle(sylar::http::HttpRequest::ptr request
                         ,sylar::http::HttpResponse::ptr response
                         ,sylar::http::HttpSession::ptr session) override;
 private:
  /// 回调函数
  callback m_cb;
};

class IServletCreator{
 public:
  typedef std::shared_ptr<IServletCreator> ptr;
  virtual ~IServletCreator() {}
  virtual Servlet::ptr get() const = 0;
  virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
 public:
  typedef std::shared_ptr<HoldServletCreator> ptr;
  HoldServletCreator(Servlet::ptr slt)
    :m_servlet(slt){}

  Servlet::ptr get() const override{
    return m_servlet;
  }

  std::string getName() const override{
    return m_servlet->getName();
  }

 private:
  Servlet::ptr m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator{
 public:
  typedef std::shared_ptr<ServletCreator> ptr;

  ServletCreator() { }

  Servlet::ptr get() const override {
    return Servlet::ptr (new T);
  }

};


/**
 * @brief Servlet分发器
 */
class ServletDispatch : public Servlet{
 public:
  /// 智能指针
  typedef std::shared_ptr<ServletDispatch> ptr;
  typedef sylar::RWMutex RWMutexType;

  /**
   * @brief 构造函数
   */
  ServletDispatch();
  virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         ,sylar::http::HttpResponse::ptr response
                         ,sylar::http::HttpSession::ptr session) override;

  void addServlet(const std::string& uri, Servlet::ptr slt);

  void addServlet(const std::string& uri,FunctionServlet::callback cb);

  void addGlobServlet(const std::string& uri,Servlet::ptr slt);

  void addGlobServlet(const std::string& uri,FunctionServlet::callback cb);

  void delServlet(const std::string& uri);

  void delGlobServlet(const std::string& uri);

  Servlet::ptr getDefault() const { return m_default; }

  void setDefault(Servlet::ptr v) { m_default = v; }

  Servlet::ptr getServlet(const std::string& uri);

  Servlet::ptr getGlobServlet(const std::string& uri);

  /**
   * @brief 通过uri获取servlet
   * @param uri uri
   * @return 优先精准匹配，其次模糊匹配，最后返回默认值
   */
  Servlet::ptr getMatchServlet(const std::string& uri);

 private:
  /// 读写互斥量
  RWMutexType m_mutex;
  /// 精准匹配servlet MAP
  /// uri(/sylar/xxx) -> servlet
  std::unordered_map<std::string,Servlet::ptr> m_datas;
  /// 模糊匹配serlvet 数组
  /// uri(/sylar/*) -> servlet
  std::vector<std::pair<std::string,Servlet::ptr>> m_globs;
  /// 默认的servlet，所有路径都没匹配到时使用
  Servlet::ptr m_default;
};

/**
 * @brief NotFounfServlt(默认返回404)
 */
class NotFounfServlet : public Servlet{
 public:
  typedef std::shared_ptr<NotFounfServlet> ptr;
  /**
   * @brief 构造函数
   */
  NotFounfServlet(const std::string& name);
  virtual int32_t handle(sylar::http::HttpRequest::ptr request
                         ,sylar::http::HttpResponse::ptr response
                         ,sylar::http::HttpSession::ptr session) override;

 private:
  std::string m_name;
  std::string m_content;
};

}
}

#endif