#include <unistd.h>
#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
int count = 0;

sylar::Mutex s_mutex;

void fun1() {
  SYLAR_LOG_INFO(g_logger) << "thread_id" << sylar::Thread::GetName()
                           << " this.name: "
                           << sylar::Thread::GetThis()->getName()
                           << " id:" << sylar::GetThreadId() << "this.id"
                           << sylar::Thread::GetThis()->getId();
  for (int i = 0; i < 100000; ++i) {
    sylar::Mutex::Lock lock(s_mutex);
    ++count;
  }
}

void fun2() {
  while (true) {
    SYLAR_LOG_INFO(g_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  }
}
void fun3() {
  while (true) {
    SYLAR_LOG_INFO(g_logger) << "==========================================";
  }
}

int main() {
  SYLAR_LOG_INFO(g_logger) << "thread test begin";
  YAML::Node root =
      YAML::LoadFile("/home/li/project/sylar/bin/conf/log2.yml");
  sylar::Config::LoadFromYaml(root);

  std::vector<sylar::Thread::ptr> thrs;
  for (int i = 0; i < 2; ++i) {
    sylar::Thread::ptr thr(
        new sylar::Thread(&fun2, "name_" + std::to_string(i * 2)));
    sylar::Thread::ptr thr1(
        new sylar::Thread(&fun3, "name_" + std::to_string(i * 2 + 1)));
    thrs.push_back(thr);
    thrs.push_back(thr1);
  }

  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->join();
  }
  SYLAR_LOG_INFO(g_logger) << "thread test end";
  SYLAR_LOG_INFO(g_logger) << "count=" << count;
  return 0;
}