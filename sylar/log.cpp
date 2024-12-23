#include "log.h"
#include <functional>
#include <iostream>
#include <map>
#include "config.h"

namespace sylar {

const char* LogLevel::ToString(LogLevel::Level level) {
  switch (level) {
#define xx(name)       \
  case LogLevel::name: \
    return #name;      \
    break;
    xx(DEBUG);
    xx(INFO);
    xx(WARN);
    xx(ERROR);
    xx(FATAL);
#undef xx
    default:
      return "UNKNOW";
  }
  return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define XX(level, v)        \
  if (str == #v) {          \
    return LogLevel::level; \
  }
  XX(DEBUG, debug);
  XX(INFO, info);
  XX(WARN, warn);
  XX(ERROR, error);
  XX(FATAL, fatal);

  XX(DEBUG, DEBUG);
  XX(INFO, INFO);
  XX(WARN, WARN);
  XX(ERROR, ERROR);
  XX(FATAL, FATAL);
  return LogLevel::UNKNOW;
#undef XX
}

LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e) {}

LogEventWrap::~LogEventWrap() {
  m_event->getLogger()->log(m_event->getLevel(), m_event);
}

void LogEvent::format(const char* fmt, ...) {
  va_list al;
  va_start(al, fmt);
  format(fmt, al);
  va_end(al);
}
void LogEvent::format(const char* fmt, va_list al) {
  char* buf = nullptr;
  int len = vasprintf(&buf, fmt, al);
  if (len != -1) {
    m_ss << std::string(buf, len);
    free(buf);
  }
}

std::stringstream& LogEventWrap::getSS() {
  return m_event->getSS();
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  m_formatter = val;
  if (m_formatter) {
    m_hasFormatter = true;
  } else {
    m_hasFormatter = false;
  }
}

LogFormatter::ptr LogAppender::getFormatter() {
  MutexType::Lock lock(m_mutex);
  return m_formatter;
}

class MessageFormatItem : public LogFormatter::FormatItem {
 public:
  MessageFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem {
 public:
  LevelFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << LogLevel::ToString(level);
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem {
 public:
  ElapseFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getElapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem {
 public:
  NameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getLogger()->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem {
 public:
  ThreadIdFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getThreadId();
  }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem {
 public:
  ThreadNameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getThreadName();
  }
};

class FiberIdFormatItem : public LogFormatter::FormatItem {
 public:
  FiberIdFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getFiberId();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem {
 public:
  DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S")
      : m_format(format) {
    if (m_format.empty()) {
      m_format = "%Y-%m-%d %H:%M:%S";
    }
  }
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    struct tm tm;
    time_t time = event->getTime();
    localtime_r(&time, &tm);
    char buf[64];
    strftime(buf, sizeof(buf), m_format.c_str(), &tm);
    os << buf;
  }

 private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem {
 public:
  FilenameFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem {
 public:
  LineFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem {
 public:
  NewLineFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem {
 public:
  StringFormatItem(const std::string& str) : m_string(str) {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << m_string;
  }

 private:
  std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
 public:
  TabFormatItem(const std::string& str = "") {}
  void format(std::ostream& os, Logger::ptr logger, LogLevel::Level level,
              LogEvent::ptr event) override {
    os << "\t";
  }

 private:
  std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> Logger, LogLevel::Level level,
                   const char* file, int32_t m_line, uint32_t elapse,
                   uint32_t threadId, uint32_t fiberId, uint64_t time,
                   const std::string& thread_name)
    : m_file(file),
      m_line(m_line),
      m_threadId(threadId),
      m_fiberId(fiberId),
      m_time(time),
      m_elapse(elapse),
      m_thread_name(thread_name),
      m_logger(Logger),
      m_level(level) {}

Logger::Logger(const std::string& name)
    : m_name(name), m_level(LogLevel::DEBUG) {
  /// 设置日志格式化器的格式
  m_formatter.reset(new LogFormatter(
      "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
};

void Logger::setFormatter(LogFormatter::ptr val) {
  MutexType::Lock lock(m_mutex);
  m_formatter = val;
  for (auto& i : m_appenders) {
    MutexType::Lock ll(i->m_mutex);
    if (!i->m_hasFormatter) {
      i->m_formatter = m_formatter;
    }
  }
}
void Logger::setFormatter(const std::string& val) {
  sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
  if (new_val->isError()) {
    std::cout << "Logger setFormatter name=" << m_name << "value= " << val
              << "invalid formatter" << std::endl;
    return;
  }
  //  m_formatter = new_val;
  setFormatter(new_val);
}
std::string Logger::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["name"] = m_name;
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  for (auto& i : m_appenders) {
    node["appenders"].push_back(YAML::Load(i->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::ptr Logger::getFormatter() {
  return m_formatter;
}

void Logger::addAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  if (!appender->getFormatter()) {
    MutexType::Lock ll(appender->m_mutex);
    appender->m_formatter = m_formatter;
  }
  m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender) {
  MutexType::Lock lock(m_mutex);
  for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
    if (*it == appender) {
      m_appenders.erase(it);
      break;
    }
  }
}
void Logger::clearAppenders() {
  MutexType ::Lock lock(m_mutex);
  m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    auto self = shared_from_this();
    //    MutexType::Lock lock(m_mutex);
    if (!m_appenders.empty()) {
      for (auto& i : m_appenders) {
        i->log(self, level, event);
      }
    } else if (m_root) {
      m_root->log(level, event);
    }
  }
}

void Logger::debug(LogEvent::ptr event) {
  log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
  log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
  log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
  log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
  log(LogLevel::FATAL, event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    : m_filename(filename) {
  reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,
                          LogEvent::ptr event) {
  if (level >= m_level) {
    uint64_t now = time(0);
    if (now >= (m_lastTime + 3)) {
      reopen();
      m_lastTime = now;
    }
    MutexType::Lock lock(m_mutex);
    if (!m_formatter->format(m_filestream, logger, level, event)) {
      std::cout << "error" << std::endl;
    }
  }
}

std::string FileLogAppender::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

bool FileLogAppender::reopen() {
  MutexType::Lock lock(m_mutex);
  if (m_filestream) {
    m_filestream.close();
  }
  m_filestream.open(m_filename);
  return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,
                            LogLevel::Level level, LogEvent::ptr event) {
  if (level >= m_level) {
    MutexType::Lock lock(m_mutex);
    std::cout << m_formatter->format(logger, level, event);
  }
}
std::string StdoutLogAppender::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  if (m_level != LogLevel::UNKNOW) {
    node["level"] = LogLevel::ToString(m_level);
  }
  if (m_hasFormatter && m_formatter) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern) : m_pattern(pattern) {
  init();
}

std::ostream& LogFormatter::format(std::ostream& ofs,
                                   std::shared_ptr<Logger> logger,
                                   LogLevel::Level level, LogEvent::ptr event) {
  for (auto& i : m_items) {
    i->format(ofs, logger, level, event);
  }
  return ofs;
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger,
                                 LogLevel::Level level, LogEvent::ptr event) {
  std::stringstream ss;
  for (auto& i : m_items) {
    i->format(ss, logger, level, event);
  }
  return ss.str();
}

void LogFormatter::init() {
  // std, format,type
  std::vector<std::tuple<std::string, std::string, int>> vec;
  /// 这个是用来存储普通文本的
  std::string nstr;
  for (std::size_t i = 0; i < m_pattern.size(); ++i) {
    /// 这个是如果它的前面没有%的话先将当前的i位置的文本加上去
    if (m_pattern[i] != '%') {
      nstr.append(1, m_pattern[i]);
      continue;
    }
    /// 下面才是有了%之后的逻辑
    /// 这个情况就是将%%其中的第二个%加到普通文本里面去
    if ((i + 1) < m_pattern.size()) {
      if (m_pattern[i + 1] == '%') {
        nstr.append(1, '%');
        continue;
      }
    }


    std::size_t n = i + 1;
    /// 这个是用来表明当前是否进入了编辑fmt阶段，就是%d对应的{}里面的部分
    int fmt_status = 0;
    /// 记录格式化项开始的位置 ：其中的例子就是%d{%Y-%m-%d %H:%M:%S} %m 其中fmt_begin就代表{的位置
    std::size_t fmt_begin = 0;
    /// str就代表上面的 d
    std::string str;
    /// fmt就代表%Y-%m-%d %H:%M:%S
    std::string fmt;
    while (n < m_pattern.size()) {
      if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' &&
                          m_pattern[n] != '}')) {
        /// 这个情况是当n走到下一个非字母、{、}的时候，然后才开始将str提取出来
        str = m_pattern.substr(i + 1, n - i - 1);
        break;
      }
      if (fmt_status == 0) {
        /// 下面这个是用来解析当前%d的对应日志格式的
        if (m_pattern[n] == '{') {
          str = m_pattern.substr(i + 1, n - i - 1);
          //                    std::cout << "*" << fmt << std::endl;
          fmt_status = 1;  //解析格式
          fmt_begin = n;
          ++n;
          continue;
        }
      } else if (fmt_status == 1) {
        if (m_pattern[n] == '}') {
          /// 这个就是直接将日志格式取出来
          fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
          //                    std::cout << "#" << fmt << std::endl;
          fmt_status = 0;
          ++n;
          break;
        }
      }
      ++n;
      if (n == m_pattern.size()) {
        if (str.empty()) {
          /// 这个是将日志的%d对应的d取出来
          str = m_pattern.substr(i + 1);
        }
      }
    }
    /// 下面就是将日志的格式插入到vec里面去
    if (fmt_status == 0) {
      /// 这个部分是将文本类插入进去
      if (!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, std::string(), 0));
        nstr.clear();
      }
      /// 这个部分就是将对应的格式话类插入进去
      vec.push_back(std::make_tuple(str, fmt, 1));
      i = n - 1;
    } else if (fmt_status == 1) {
      /// 这个情况是因为如果mt_status在结束这个while之后还是等于1代表其中的格式解析部分是错的。
      /// 所以需要将其格式解析部分提取出来进行判错
      std::cout << "pattern parse error : " << m_pattern << "-"
                << m_pattern.substr(i) << std::endl;
      m_error = true;
      vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
    }
  }
    /// 上面是for循环的结束

    /// 下面这个是对文本类的收尾
  if (!nstr.empty()) {
    vec.push_back(std::make_tuple(nstr, "", 0));
  }

  /// 是一个静态映射，将字符串映射到相应的日志格式化类
  static std::map<std::string,
                  std::function<FormatItem::ptr(const std::string& str)>>
      s_format_items = {
#define XX(str, C)                        \
  {                                       \
    #str, [](const std::string& fmt) {    \
      return FormatItem::ptr(new C(fmt)); \
    }                                     \
  }

        XX(m, MessageFormatItem),     //消息
        XX(p, LevelFormatItem),       //日志级别
        XX(r, ElapseFormatItem),      //累计毫秒数
        XX(c, NameFormatItem),        //日志名称
        XX(t, ThreadIdFormatItem),    //线程id
        XX(n, NewLineFormatItem),     //换行
        XX(d, DateTimeFormatItem),    //时间
        XX(f, FilenameFormatItem),    //文件名
        XX(l, LineFormatItem),        //行号
        XX(T, TabFormatItem),         //Tab
        XX(F, FiberIdFormatItem),     //协程id
        XX(N, ThreadNameFormatItem),  //线程名称
#undef XX
      };
  /// 这个就是根据上面的vec部分保留的格式开始创建对应的FormatItem，日志格式化解析类
  for (auto& i : vec) {
    /// 这一部分代表着是0，0就是普通的字符串
    if (std::get<2>(i) == 0) {
      m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
    } else {
      /// 这个对应的就是日志格类
      auto it = s_format_items.find(std::get<0>(i));
      if (it == s_format_items.end()) {
        m_items.push_back(FormatItem::ptr(
            new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
        m_error = true;
      } else {
        m_items.push_back(it->second(std::get<1>(i)));
      }
    }
    //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" <<std::endl;
  }
  //std::cout << m_items.size() << std::endl;
}

LoggerManager::LoggerManager() {
  m_root.reset(new Logger);
  m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
  m_loggers[m_root->m_name] = m_root;
  init();
}

std::string LoggerManager::toYamlString() {
  MutexType::Lock lock(m_mutex);
  YAML::Node node;
  for (auto& i : m_loggers) {
    node.push_back(YAML::Load(i.second->toYamlString()));
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
  MutexType::Lock lock(m_mutex);
  auto it = m_loggers.find(name);
  if (it != m_loggers.end()) {
    return it->second;
  }
  Logger::ptr logger(new Logger(name));
  logger->m_root = m_root;
  m_loggers[name] = logger;
  return logger;
}

struct LogAppenderDefine {
  int type = 0;  //1 File, 2 Stdout
  LogLevel::Level m_level = LogLevel::UNKNOW;
  std::string formatter;
  std::string file;

  bool operator==(const LogAppenderDefine& oth) const {
    return type == oth.type && m_level == oth.m_level &&
           formatter == oth.formatter && file == oth.file;
  }
};

struct LogDefine {
  std::string name;
  LogLevel::Level m_level = LogLevel::UNKNOW;
  std::string formatter;
  std::vector<LogAppenderDefine> appenders;
  bool operator==(const LogDefine& oth) const {
    return name == oth.name && m_level == oth.m_level &&
           formatter == oth.formatter && appenders == oth.appenders;
  }
  bool operator<(const LogDefine& oth) const { return name < oth.name; }
};

template <>
class LexicalCast<std::string, std::set<LogDefine>> {
 public:
  std::set<LogDefine> operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    std::set<LogDefine> vec;
    for (size_t i = 0; i < node.size(); ++i) {
      auto n = node[i];
      if (!n["name"].IsDefined()) {
        std::cout << "log config error: name is null," << n << std::endl;
        continue;
      }
      LogDefine ld;
      ld.name = n["name"].as<std::string>();
      ld.m_level = LogLevel::FromString(
          n["level"].IsDefined() ? n["level"].as<std::string>() : "");
      if (n["formatter"].IsDefined()) {
        ld.formatter = n["formatter"].as<std::string>();
      }
      if (n["appenders"].IsDefined()) {
        for (size_t x = 0; x < n["appenders"].size(); ++x) {
          auto a = n["appenders"][x];
          if (!a["type"].IsDefined()) {
            std::cout << "log config error: appender type is null," << a
                      << std::endl;
            continue;
          }
          std::string type = a["type"].as<std::string>();
          LogAppenderDefine lad;
          if (type == "FileLogAppender") {
            lad.type = 1;
            if (!a["file"].IsDefined()) {
              std::cout << "log config error: fileappender is null," << a
                        << std::endl;
              continue;
            }
            lad.file = a["file"].as<std::string>();
            if (a["formatter"].IsDefined()) {
              lad.formatter = a["formatter"].as<std::string>();
            }
          } else if (type == "StdoutLogAppender") {
            lad.type = 2;
          } else {
            std::cout << "log config error: name is invalid," << a << std::endl;
            continue;
          }
          ld.appenders.push_back(lad);
        }
      }
      //          std::cout << "----" << ld.name << "-"
      //                    << ld.appenders.size() << std::endl;
      vec.insert(ld);
    }
    return vec;
  }
};

template <>
class LexicalCast<std::set<LogDefine>, std::string> {
 public:
  std::string operator()(const std::set<LogDefine>& v) {
    YAML::Node node;
    for (auto& i : v) {
      YAML::Node n;
      n["name"] = i.name;
      if (i.m_level != LogLevel::UNKNOW) {
        n["level"] = LogLevel::ToString(i.m_level);
      }
      if (i.formatter.empty()) {
        n["formatter"] = i.formatter;
      }
      for (auto& a : i.appenders) {
        YAML::Node na;
        if (a.type == 1) {
          na["type"] = "FileLogAppender";
          na["file"] = a.file;
        } else if (a.type == 2) {
          na["type"] = "StdoutLogAppender";
        }
        if (a.m_level != LogLevel::UNKNOW) {
          na["level"] = LogLevel::ToString(a.m_level);
        }
        if (!a.formatter.empty()) {
          na["formatter"] = a.formatter;
        }

        n["appenders"].push_back(na);
      }
      node.push_back(n);
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
    sylar::Config::Lookup("logs", std::set<LogDefine>(), "log config");

struct LogIniter {
  LogIniter() {
    g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                                  const std::set<LogDefine>& new_value) {
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
      //新增
      for (auto& i : new_value) {
        auto it = old_value.find(i);
        sylar::Logger::ptr logger;
        if (it == old_value.end()) {
          //新增logger
          logger = SYLAR_LOG_NEAME(i.name);
        } else {
          if (!(i == *it)) {
            //修改的logger
            logger = SYLAR_LOG_NEAME(i.name);
          }
        }
        logger->setLevel(i.m_level);
        if (!i.formatter.empty()) {
          logger->setFormatter(i.formatter);
        }
        logger->clearAppenders();
        for (auto& a : i.appenders) {
          sylar::LogAppender::ptr ap;
          if (a.type == 1) {
            ap.reset(new FileLogAppender(a.file));
          } else if (a.type == 2) {
            ap.reset(new StdoutLogAppender);
          }
          ap->setLevel(a.m_level);
          if (!a.formatter.empty()) {
            LogFormatter::ptr fmt(new LogFormatter(a.formatter));
            if (!fmt->isError()) {
              ap->setFormatter(fmt);
            } else {
              std::cout << "appender name =" << i.name
                        << "appender type=" << a.type
                        << "formatter=" << a.formatter << "is invalid"
                        << std::endl;
            }
          }
          logger->addAppender(ap);
        }
      }
      for (auto& i : old_value) {
        auto it = new_value.find(i);
        if (it == new_value.end()) {
          //删除logger
          auto logger = SYLAR_LOG_NEAME(i.name);
          logger->setLevel((LogLevel::Level)100);
          logger->clearAppenders();
        }
      }
    });
  }
};

static LogIniter __log_init;
void LoggerManager::init() {}

}  // namespace sylar
