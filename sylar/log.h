#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <stdarg.h>
#include <stdint.h>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "singleton.h"
#include "thread.h"
#include "util.h"

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 * */

#define SYLAR_LOG_LEVEL(logger, level)                                \
  if (logger->getLevel() <= level)                                    \
  sylar::LogEventWrap(                                                \
      sylar::LogEvent::ptr(new sylar::LogEvent(                       \
          logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
          sylar::GetFiberId(), time(0), sylar::Thread::GetName())))   \
      .getSS()

#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

/**
 * 使用格式化的方式将日志级别level的日志写入到logger
 * */

#define SYLAR_LOG_FMT_LEVEL(logger, level, fmt, ...)                  \
  if (logger->getLevel() <= level)                                    \
  sylar::LogEventWrap(                                                \
      sylar::LogEvent::ptr(new sylar::LogEvent(                       \
          logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), \
          sylar::GetFiberId(), time(0), sylar::Thread::GetName())))   \
      .getEvent()                                                     \
      ->format(fmt, __VA_ARGS__)

#define SYLAR_LOG_FMT_DEBUG(logger, fmt, ...) \
  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_INFO(logger, fmt, ...) \
  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_WARN(logger, fmt, ...) \
  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__)
#define SYLAR_LOG_FMT_ERROR(logger, fmt, ...) \
  SYLAR_LOG_FMT_LEVEL(logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__)

/**
 * @brief 获取主日志器
 * */

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

/**
 * @brief 获取name的日志器
 * */

#define SYLAR_LOG_NEAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar {
class Logger;
//日志事件

/**
 * @brief 日志级别
 * */

class LogLevel {
 public:
  enum Level {
    /// 未知级别
    UNKNOW = 0,
    /// DEBUG 级别
    DEBUG = 1,
    /// INFO级别
    INFO = 2,
    /// WARN 级别
    WARN = 3,
    /// ERROR 级别
    ERROR = 4,
    /// FATAL 级别
    FATAL = 5,
  };
  /**
  * @brief 将日志级别转成文本输出
  * @param[in] level 日志级别
  */
  static const char* ToString(LogLevel::Level);
  /**
  * @brief 将文本转换成日志级别
  * @param[in] str 日志级别文本
  */
  static LogLevel::Level FromString(const std::string& str);
};

/**
 * @brief 日志事件
 */
class LogEvent {
 public:
  typedef std::shared_ptr<LogEvent> ptr;

  /**
     * @brief 构造函数
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] file 文件名
     * @param[in] line 文件行号
     * @param[in] elapse 程序启动依赖的耗时(毫秒)
     * @param[in] thread_id 线程id
     * @param[in] fiber_id 协程id
     * @param[in] time 日志事件(秒)
     * @param[in] thread_name 线程名称
     */
  LogEvent(std::shared_ptr<Logger> Logger, LogLevel::Level level,
           const char* file, int32_t m_line, uint32_t elapse, uint32_t threadId,
           uint32_t fiberId, uint64_t time, const std::string& thread_name);

  /**
     * @brief 返回文件名
     */
  const char* getFile() const { return m_file; }
  /**
     * @brief 返回行号
     */
  int32_t getLine() const { return m_line; }
  /**
     * @brief 返回耗时
     */
  uint32_t getElapse() const { return m_elapse; }
  /**
     * @brief 返回线程ID
     */
  uint32_t getThreadId() const { return m_threadId; }
  /**
     * @brief 返回协程ID
     */
  uint32_t getFiberId() const { return m_fiberId; }
  /**
     * @brief 返回时间
     */
  uint64_t getTime() const { return m_time; }
  /**
     * @brief 返回线程名称
     */
  const std::string& getThreadName() const { return m_thread_name; }
  /**
     * @brief 返回日志内容
     */
  std::string getContent() const { return m_ss.str(); }
  /**
     * @brief 返回日志内容字符串流
     */
  std::stringstream& getSS() { return m_ss; }
  /**
     * @brief 返回日志器
     */
  std::shared_ptr<Logger> getLogger() const { return m_logger; }
  /**
     * @brief 返回日志级别
     */
  LogLevel::Level getLevel() const { return m_level; }
  /**
     * @brief 格式化写入日志内容
     */
  void format(const char* fmt, ...);
  /**
     * @brief 格式化写入日志内容
     */
  void format(const char* fmt, va_list al);

 private:
  /// 文件名
  const char* m_file = nullptr;
  /// 行号
  int32_t m_line = 0;
  /// 线程号
  uint32_t m_threadId = 0;
  /// 协程id
  uint32_t m_fiberId = 0;
  /// 时间戳
  uint64_t m_time = 0;
  /// 耗时
  uint32_t m_elapse = 0;
  /// 线程名
  std::string m_thread_name;
  /// 日志内容流
  std::stringstream m_ss;
  /// 日志器
  std::shared_ptr<Logger> m_logger;
  /// 日志等级
  LogLevel::Level m_level;
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap {
 public:
  /**
     * @brief 构造函数
     * @param[in] e 日志事件
     */
  LogEventWrap(LogEvent::ptr e);
  /**
     * @brief 析构函数
     */
  ~LogEventWrap();

  /**
     * @brief 获取日志事件
     */
  LogEvent::ptr getEvent() const { return m_event; }
  /**
     * @brief 获取日志内容流
     */
  std::stringstream& getSS();

 private:
  /**
     * @brief 日志事件
     */
  LogEvent::ptr m_event;
};

/**
 * @brief 日志格式化
 */

class LogFormatter {
 public:
  typedef std::shared_ptr<LogFormatter> ptr;

  /**
     * @brief 构造函数
     * @param[in] pattern 格式模板
     * @details
     *  %m 消息
     *  %p 日志级别
     *  %r 累计毫秒数
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *
     *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
     */
  LogFormatter(const std::string& pattern);

  /**
     * @brief 返回格式化日志文本
     * @param[in] logger 日志器
     * @param[in] level 日志级别
     * @param[in] event 日志事件
     */
  std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level,
                     LogEvent::ptr event);
  std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger,
                       LogLevel::Level level, LogEvent::ptr event);

 public:
  /**
   * @brief 日志内容项格式化
   */
  class FormatItem {
   public:
    typedef std::shared_ptr<FormatItem> ptr;
    /**
     * @brief 析构函数
     */
    virtual ~FormatItem() {}
    /**
     * @brief 格式化日志到流
     * @param[in, out] os 日志输出流
     * @param[in] logger 日志器
     * @param[in] level 日志等级
     * @param[in] event 日志事件
     */
    virtual void format(std::ostream& os, std::shared_ptr<Logger> logger,
                        LogLevel::Level level, LogEvent::ptr event) = 0;
  };
  /**
   * @brief 初始化,解析日志模板
   */
  void init();

  /**
   * @brief 是否有错误
   */
  bool isError() const { return m_error; }

  /**
   * @brief 返回日志模板
   */
  const std::string& getPattern() const { return m_pattern; }

 private:
  /// 日志格式解析后格式
  std::vector<FormatItem::ptr> m_items;
  /// 日志格式模板
  std::string m_pattern;
  /// 是否有错误
  bool m_error = false;
};

//日志输出地
class LogAppender {
  friend class Logger;

 public:
  typedef std::shared_ptr<LogAppender> ptr;
  typedef SpinLock MutexType;
  virtual ~LogAppender() {}

  virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                   LogEvent::ptr event) = 0;
  virtual std::string toYamlString() = 0;

  void setFormatter(LogFormatter::ptr val);
  LogFormatter::ptr getFormatter();

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level val) { m_level = val; }

 protected:
  LogLevel::Level m_level = LogLevel::DEBUG;
  bool m_hasFormatter = false;
  MutexType m_mutex;
  LogFormatter::ptr m_formatter;
};

//日志输出器
class Logger : public std::enable_shared_from_this<Logger> {
  friend class LoggerManager;

 public:
  typedef std::shared_ptr<Logger> ptr;
  typedef SpinLock MutexType;
  Logger(const std::string& name = "root");

  void log(LogLevel::Level level, const LogEvent::ptr event);

  void debug(LogEvent::ptr event);
  void info(LogEvent::ptr event);
  void warn(LogEvent::ptr event);
  void error(LogEvent::ptr event);
  void fatal(LogEvent::ptr event);

  void addAppender(LogAppender::ptr appender);
  void delAppender(LogAppender::ptr appender);
  void clearAppenders();
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel(LogLevel::Level val) { m_level = val; }
  const std::string& getName() const { return m_name; }

  void setFormatter(LogFormatter::ptr val);
  void setFormatter(const std::string& val);
  LogFormatter::ptr getFormatter();
  std::string toYamlString();

 private:
  std::string m_name;                       //日志名称
  LogLevel::Level m_level;                  //日志级别
  MutexType m_mutex;                        //日志互斥锁
  std::list<LogAppender::ptr> m_appenders;  //Appender集合:输出到目的地的集合
  LogFormatter::ptr m_formatter;            //日志格式器
  Logger::ptr m_root;                       //根日志对象
};
//输出控制台的Appender
class StdoutLogAppender : public LogAppender {
 public:
  typedef std::shared_ptr<StdoutLogAppender> ptr;
  void log(Logger::ptr logger, LogLevel::Level level,
           LogEvent::ptr event) override;
  std::string toYamlString() override;

 private:
};

//输出文件的Appender
class FileLogAppender : public LogAppender {
 public:
  typedef std::shared_ptr<FileLogAppender> ptr;
  FileLogAppender(const std::string& filename);
  void log(std::shared_ptr<Logger> logger, LogLevel::Level level,
           LogEvent::ptr event) override;
  std::string toYamlString() override;
  //重新打开文件，文件打开成功返回true
  bool reopen();

 private:
  std::string m_filename;
  std::ofstream m_filestream;
  uint64_t m_lastTime = 0;
};

class LoggerManager {
 public:
  typedef SpinLock MutexType;
  LoggerManager();
  Logger::ptr getLogger(const std::string& name);
  void init();
  Logger::ptr getRoot() const { return m_root; }
  std::string toYamlString();

 private:
  MutexType m_mutex;
  std::map<std::string, Logger::ptr> m_loggers;
  Logger::ptr m_root;
};

typedef sylar::Singleton<LoggerManager> LoggerMgr;

}  // namespace sylar

#endif
