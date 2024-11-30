#include "log.h"
#include <map>
#include <functional>
#include <iostream>

namespace sylar{

const char* LogLever::ToString(LogLever::Level){
    switch (Level){
#define XX(name) \
    case LogLever::name: \
        return #name; \
        break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

Logger::Logger(const std::string& name)
    :m_name(name){

};

void Logger::addAppender(LogAppender::ptr appender){
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin();
            it != m_appenders.end(); ++it){
                if(*it == appender){
                    m_appenders.erase(it);
                    break;
            }
    }
}

void Logger::log(LogLever::Level level,const LogEvent::ptr event){
    if(level >= m_level){
        for(auto& i : m_appenders){
            i->log(level,event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    log(LogLever::DEBUG,event);
}

void Logger::info(LogEvent::ptr event){
    log(LogLever::INFO,event);
}

void Logger::warn(LogEvent::ptr event){
    log(LogLever::WARN,event);
}

void Logger::error(LogEvent::ptr event){
    log(LogLever::ERROR,event);
}

void Logger::fatal(LogEvent::ptr event){
    log(LogLever::FATAL,event);
}

FileLogAppender::FileLogAppender(const std::string& filename)
    :m_filename(filename){}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event)  {
    if(level >= m_level){
        m_filestream << m_formatter->format(logger,level,event);
    }
}
bool FileLogAppender::reopen(){
    if(m_filestream){
        m_filestream.close();
    }
    
    m_filestream.open(m_filestream);
    return !!m_filename;
}

void StdoutLogAppender::log(LogLever::Level level,LogEvent::ptr event){
    if(level >= m_level){
        std::cout << m_formatter->format(event);
    }
}

LogFormatter::LogFormatter(const std::string& pattern)
    :m_pattern(pattern) {

}

std::string LogFormatter::format(LogLever::Level level, LogEvent::ptr event){
    std::stringstream ss;
    for(auto& i : m_items){
        i->format(ss,level,event);
    }
    return ss.str();
}   

void LogFormatter::init(){
    // std, format,type
    std::vector<std::tuple<std::string,std::string,int>> vec;
    std::string nstr;
    for(std::size_t i = 0; i < m_pattern.size();++i){
        if(m_pattern[i] == '%'){
            nstr.append(1,m_pattern[i]);
            continue;
        }
        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i+1] == '%'){
                nstr.append(1,'%');
                continue;
            }
        }

        std::size_t n = i+1;
        int fmt_status = 0;
        std::size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()){
            if(isspace(m_pattern[n])){
                break;
            }
            if(fmt_status == 0){
                if(m_pattern[n] == '{'){
                    str = m_pattern.substr(i + 1, n - i);
                    fmt_status = 1; //解析格式
                    ++n;
                    fmt_begin = n;
                    continue;
                }
            }
            if(fmt_status == 1){
                if(m_pattern[n] == '}'){
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    fmt_status = 2;
                    break;
                }
            }
        }
        if(fmt_status == 0){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,"",0));
            }
            str = m_pattern.substr(i + 1, n - i - 1);
            vec.push_back(std::make_tuple(str,fmt,1));
            i = n;
        } else if(fmt_status == 1){
            std::cout<<"pattern parse error : " << m_pattern << "-" <<m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<<pattern_error>>",fmt,0));
        } else if(fmt_status == 2){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,"",0));
            }
            vec.push_back(std::make_tuple(str,fmt,1));
            i = n;
        }
    }
    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr,"",0));
    }
    //m:消息
    //p:日志级别
    //r:累计毫秒数
    //c:日志名称
    //t:线程id
    //n:换行
    //d:时间
    //f:文件名
    //l:行号

}

class MessageFormatItem : public LogFormatter::FormatItem {
public:
    MessageFormatItem(const std::string& str = "") {}
    void format(std::ostream& os,LogLever::Level level,LogEvent::ptr event) override {
        os << event->getContent();
    }
};


class LevelFormatItem : public LogFormatter::FormatItem{
public:
    void format(std::ostream& os,LogLever::Level level,LogEvent::ptr event) override{
        os << LogLever::ToString(level);
    }
};
class ElapseFormatItem : public LogFormatter::FormatItem{

}

}

