#include <cstdio>
#include <stdlib.h>
#include <cstdarg>
#include <iostream>
#include <cstdint>
#include <string>
#include <memory>
#include <boost/version.hpp>
#include <boost/config.hpp>
#include "iostream"
#include "sylar/util.h"
#include <functional>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAXBUF 1024
#define MAX_CLIENTS 5

namespace test_example_function
{
    void format(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
      
        char* buf = nullptr;
        int len = vasprintf(&buf, fmt, args);
        if (len != -1) {
          std::cout << std::string(buf, len) << std::endl;
          free(buf);
        }
      
        va_end(args);
    }

    void test_format() {
        const char* formatString = "Hello, %s! The answer is %d.";
        const char* name = "World";
        int answer = 42;
      
        // 使用 va_list 来传递可变参数
        format(formatString, name, answer);
    }
} // namespace test_example_function

namespace test_log_example{
  class A {
    public:
     A() : m_a(0) {
       std::cout << "A()" << std::endl;
     }
     
     ~A() {
       std::cout << "~A()" << std::endl;
     }
   
     int* get_a() {
       return &m_a;
     }
   
   private:
     int m_a;
   };
   
   
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
   
   class LogLevel{
     public:
         enum Level{
             UNKNOW = 0, //起手先来个未知级别兜底
             DEBUG = 1,  //调试级别
             INFO = 2,   //普通信息级别
             WARN = 3,   //警告信息
             ERROR = 4,  //错误信息
             FATAL = 5   //灾难级信息
         };
     
       static const char* ToString(LogLevel::Level level);
     };
   const char* LogLevel::ToString(LogLevel::Level level){
         switch(level) {
     #define XX(name) \
         case LogLevel::name: \
             return #name; \
             break;
             
         XX(DEBUG);
         XX(INFO);
         XX(WARN);
         XX(ERROR);
         XX(FATAL);
         
     #undef XX
         default:
             return "UNKNOW";
         }
         return "UNKNOW";
   }
}

namespace test_example_config
{

template <class F, class T>
class LexicalCast {
public:
  T operator()(const F& v) const { return boost::lexical_cast<T>(v); }
};
  


template <class T>
class LexicalCast<std::vector<T>, std::string> {
   public:
    std::string operator()(const std::vector<T>& v) {
      YAML::Node node;
      for (auto& i : v) {
        node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
      }
      std::stringstream ss;
      ss << node;
      return ss.str();
  }
};

template <class T>
class LexicalCast<std::string, std::vector<T>> {
 public:
  std::vector<T> operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    typename std::vector<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::string, std::list<T>> {
 public:
  std::list<T> operator()(const std::string& v) {
    YAML::Node node = YAML::Load(v);
    typename std::list<T> vec;
    std::stringstream ss;
    for (size_t i = 0; i < node.size(); i++) {
      ss.str("");
      ss << node[i];
      vec.push_back(LexicalCast<std::string, T>()(ss.str()));
    }
    return vec;
  }
};

template <class T>
class LexicalCast<std::list<T>, std::string> {
 public:
  std::string operator()(const std::list<T>& v) {
    YAML::Node node;
    for (auto& i : v) {
      node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};


} // namespace test_example_config

namespace test_thread{
void func0(){
    std::cout << "This is func0 " << std::endl;
}

void func1(){
    std::cout << "This is func1" << std::endl;
}

}

namespace test_fiber {
//此方法目的是将方法的堆栈调用以字符串数组的形式存放到vector中
//size是指想要看到的最大堆栈调用层级
//skip是为了简化不必要的前置调用堆栈用的，默认可以是2
void Backtrace(std::vector<std::string>& bt, int size, int skip) {
  void** array = (void**)malloc(size * sizeof(void*));
  size_t s = ::backtrace(array, size);

  char** strings = backtrace_symbols(array, s);

  if (strings == NULL) {
    std::cout << "backtrace_synbols error" << std::endl;
    return;
  }
  for (size_t i = skip; i < s; i++) {
    bt.push_back(strings[i]);
  }

  //注意内存的需要释放
  free(strings);
  free(array);
}

std::string BacktraceToString(int size, int skip, const std::string& prefix) {
  std::vector<std::string> bt;
  Backtrace(bt, size, skip);
  std::stringstream ss;
  for (size_t i = 0; i < bt.size(); i++) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}
}

namespace test_IO{
void test_select() {

}

}


int main() {
  // std::cout << "===============================" << std::endl;
  // test_example_function::test_format();
  // std::cout << "===============================" << std::endl;
  // auto b = test_log_example::SingletonPtr<test_log_example::A>::GetInstance();
  // std::cout << *b->get_a() << std::endl;
  // *b->get_a() = 10;
  // std::cout << *b->get_a() << std::endl;
  // std::cout << *b->get_a() << std::endl;
  // std::cout << "===============================" << std::endl;
  // test_log_example::LogLevel::Level level = test_log_example::LogLevel::DEBUG;
  // std::cout << test_log_example::LogLevel::ToString(level) << std::endl;
  // std::cout << "===============================" << std::endl;
  // // std::cout << BOOST_VERSION <<std::endl;
  // // std::cout << BOOST_LIB_VERSION<<std::endl; 
  // // std::cout << BOOST_STDLIB <<std::endl;
  // // std::cout <<BOOST_PLATFORM <<std::endl;
  // // std::cout << BOOST_COMPILER<<std::endl;
  // std::cout << " ================= test_example_config ===================" << std::endl;
  // // 如何利用yaml-cpp库解析yaml文件
  // YAML::Node node = YAML::LoadFile("./conf/test.yaml");
	// std::cout << node["name"].as<std::string>() << std::endl;
  //   std::cout << node["sex"].as<std::string>() << std::endl;
  //   std::cout << node["age"].as<int>() << std::endl;//18
  //   std::cout << node["system"]["port"].as<std::string>() << std::endl;
  //   std::cout << node["system"]["value"].as<std::string>() << std::endl;
  //   for(auto it = node["system"]["int_vec"].begin(); it != node["system"]["int_vec"].end(); ++it){
	// 	std::cout << *it << " ";
	// }
  // // find_first_not_of 与 find_last_not_of 的使用
  // // 1.find_first_not_of()函数正向查找在原字符串中第一个与指定字符串（或字符）中的任一字符都不匹配的字符，返回它的位置。
  // // 若查找失败，则返回npos。（npos定义为保证大于任何有效下标的值。）
  // std::string prefix = "xyz";
  // std::string str = "abcd";
  // if (prefix.find_first_not_of(str) != std::string::npos) {
  //   std::cout << "prefix is not in str" << std::endl;
  // }
  // // 2.find_last_not_of()函数
  // // 正向查找在原字符串中最后一个与指定字符串（或字符）中的任一字符都不匹配的字符，返回它的位置。
  // // 若查找失败，则返回npos。（npos定义为保证大于任何有效下标的值。）
  // if(prefix.find_last_not_of(str)!=std::string::npos){
	// 	std::cout << prefix << " is not in " << str << std::endl;
	// }
  // std::cout << "===============================" << std::endl;
  // std::vector<int> vec = {1, 2, 3, 4, 5};
  // std::string str_vec = test_example_config::LexicalCast<std::vector<int>, std::string>()(vec);
  // std::cout <<  "str_vec : " << str_vec << std::endl;
  // std::cout << "===============================" << std::endl;
  // std::string str_1 = "- 1\n- 2\n- 3";
  // std::vector<int> vec_1 = test_example_config::LexicalCast<std::string, std::vector<int>>()(str_1);
  // std :: cout << "vec_1 : ";
  // for(auto& i : vec_1){
  //   std::cout << i << " ";  
  // }
  // std::cout << std::endl;
  // std::cout << "===============================" << std::endl;
  // std::string str_list = "[1, 2, 3, 4, 5]";
  // std::list<int> list = test_example_config::LexicalCast<std::string, std::list<int>>()(str_list);
  // std :: cout << "list : ";
  // for(auto& i : list){
  //   std::cout << i << " ";
  // }
  // std::cout << std::endl;
  // std::cout << "===============================" << std::endl;
  // std::list<int> list_1 = {1, 2, 3, 4, 5};
  // std::string str_list_1 = test_example_config::LexicalCast<std::list<int>, std::string>()(list_1);
  // std::cout << "str_list_1 : " << str_list_1 << std::endl;
  // std::cout << "===============================" << std::endl;
  // std::function<void(void)> fa = test_thread::func0;
  // std::function<void(void)> fb = test_thread::func1;

  // std::cout << "====交换前====" << std::endl;
  // fa();
  // std::cout << "fa address: " << &fa << std::endl;
  // fb();
  // std::cout << "fb address: " << &fb << std::endl;

  // //开始交换
  // fa.swap(fb);

  // std::cout << "====交换后====" << std::endl;
  // fa();
  // std::cout << "fa address: " << &fa << std::endl;
  // fb();
  // std::cout << "fb address: " << &fb << std::endl;

  test_IO::test_select();


  return 0;
}



