#include "util.h"
#include "log.h"
#include <execinfo.h>

namespace sylar {
sylar::Logger::ptr g_logger = SYLAR_LOG_NEAME("system");
pid_t GetThreadId() {
  return syscall(SYS_gettid);
}
uint32_t GetFiberId() {
  return 0;
}
/**
 * 堆栈回溯（Backtrace） 的函数 Backtrace，
 * 用于获取程序运行时的调用栈信息，并将这些信息保存到一个字符串向量中
 * */
void Backtrace(std::vector<std::string>& bt,int size, int skip){
    void** array = (void **)malloc((sizeof (void *)) * size);
    size_t s = ::backtrace(array,size);
    char** strings = backtrace_symbols(array,s);
    if(strings == NULL){
      SYLAR_LOG_ERROR(g_logger) << "backtrace_synbols error";
      return ;
    }
    for(size_t i = skip; i < s; ++i){
      bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

std::string BacktraceToString(int size, int skip,const std::string& prefix){
  std::vector<std::string> bt;
  Backtrace(bt,size,skip);
  std::stringstream ss;
  for(size_t i = 0; i < bt.size(); ++i){
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

}  // namespace sylar