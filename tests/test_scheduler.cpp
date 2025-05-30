#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber() {
  static int s_count = 5;
  SYLAR_LOG_INFO(g_logger) << "test in fiber";
  sleep(1);
  if (--s_count >= 0) {
    sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
  }
}

int main(int arg, char** argv) {
  SYLAR_LOG_INFO(g_logger) << "main";
  sylar::Scheduler sc(3, false, "test");
  sc.Start();
  sleep(2);
  SYLAR_LOG_INFO(g_logger) << "schedule";
  sc.schedule(&test_fiber);
  sc.Stop();
  SYLAR_LOG_INFO(g_logger) << "over";
  return 0;
}