/**
 * @file singleton.h
 * @author lzh
 * @date 2024/12/5
 * @brief 单例模式的实现
 * */

#ifndef __SYLAR_SINGLETON_H__
#define __SYLAR_SINGLETON_H__
#include <memory>

namespace sylar {

template <typename T, class X = void, int N = 0>
class Singleton {
 public:
  static T* GetInstance() {
    static T v;
    return &v;
  }
};

template <typename T, class X = void, int N = 0>
class SingletonPtr {
 public:
  static std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> v(new T);
    return v;
  }
};

}  // namespace sylar

#endif  // __SYLAR_SINGLETON_H__