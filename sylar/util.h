#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>

namespace sylar {

pid_t GetThreadId();
uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt,int size, int skip = 1);

std::string BacktraceToString(int size, int skip = 2,const std::string& prefix = "");

}  // namespace sylar

#endif  // __SYLAR_UTIL_H__