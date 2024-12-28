#include "sylar/address.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test(){
  std::vector<sylar::Address::ptr> addrs;
  SYLAR_LOG_INFO(g_logger) << "begin";
  bool v = sylar::Address::Lookup(addrs,"www.baidu.com");
  SYLAR_LOG_INFO(g_logger) << "end";
  if(v){
    SYLAR_LOG_ERROR(g_logger) << "lookup fail";
    return;
  }
  for(size_t i = 0; i < addrs.size(); ++i){
    SYLAR_LOG_INFO(g_logger) << " - " << addrs[i]->toString();
  }
}

int main(int argc, char** argv){
  test();
  return 0;
}

