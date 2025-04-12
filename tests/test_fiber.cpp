#include "sylar/sylar.h"
#include "sylar/thread.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber() {
  SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";
  sylar::Fiber::YieldToHold();
  SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
  sylar::Fiber::YieldToHold();
}
void test_fiber() {
  SYLAR_LOG_INFO(g_logger) << "main begin - 1";
  {
    sylar::Fiber::GetThis();  //这个之后当前线程有了主协程
    SYLAR_LOG_INFO(g_logger) << "main begin";
    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "main after swapIn";
    fiber->swapIn();
    SYLAR_LOG_INFO(g_logger) << "main after end";
  }
  SYLAR_LOG_INFO(g_logger) << "main after end2";
}



int main(int argc, char** argv) {
  sylar::Thread::SetName("main");
  // std::vector<sylar::Thread::ptr> thrs;
  sylar::Thread::ptr thr(new sylar::Thread(test_fiber, "name"));
  thr->join();
  return 0;
}