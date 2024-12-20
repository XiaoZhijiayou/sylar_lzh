#include "sylar/hook.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_sleep(){
  sylar::IOManager iom(1);
  iom.schedule([](){
    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "sleep 2";
  });

  iom.schedule([](){
    sleep(3);
    SYLAR_LOG_INFO(g_logger) << "sleep 3";
  });
  SYLAR_LOG_INFO(g_logger) << "test sleep";
}

int main(int argc,char** argv){
  test_sleep();
  return 0;
}