#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
namespace sylar {

pid_t GetThreadId();
uint32_t GetFiberId();

}  // namespace sylar

#endif  // __SYLAR_UTIL_H__