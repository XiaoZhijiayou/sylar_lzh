#include "util.h"
namespace sylar {
#include <pthread.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>

pid_t GetThreadId() {
  return syscall(SYS_gettid);
}
uint32_t GetFiberId() {
  return 0;
}
}  // namespace sylar