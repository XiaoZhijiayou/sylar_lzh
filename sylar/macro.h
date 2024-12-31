
/**
 * @brief 常用宏的封装
 * */

#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <assert.h>
#include <string.h>
#include "util.h"

#if defined __GNUC__ || defined __LLVM__

#define SYLAR_LIKELY(X) __builtin_expect(!!(X), 1)

#define SYLAR_UNLIKELY(X) __builtin_expect(!!(X), 0)
#else
#define SYLAR_LIKELY(X) (X)
#define SYLAR_UNLIKELY(X) (X)
#endif

#define SYLAR_ASSERT(X)                              \
  if (SYLAR_UNLIKELY(!(X))) {                        \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                \
        << "ASSERTION: " #X << "\nbacktrace:\n"      \
        << sylar::BacktraceToString(100, 2, "    "); \
    assert(X);                                       \
  }

#define SYLAR_ASSERT2(X, W)                          \
  if (SYLAR_UNLIKELY(!(X))) {                        \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                \
        << "ASSERTION: " #X << "\n"                  \
        << W << "\nbacktrace:\n"                     \
        << sylar::BacktraceToString(100, 2, "    "); \
    assert(X);                                       \
  }

#endif /* __SYLAR_MACRO_H__ */