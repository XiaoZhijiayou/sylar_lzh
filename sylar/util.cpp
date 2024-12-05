#include "util.h"
namespace sylar{
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>

pid_t GetThreadId(){
  return syscall(SYS_gettid);
}
uint32_t GetFiberId(){
    return 0;
}
}