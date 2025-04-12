#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <boost/lexical_cast.hpp>

namespace sylar {

pid_t GetThreadId();
uint32_t GetFiberId();

void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);

std::string BacktraceToString(int size = 64, int skip = 2,
                              const std::string& prefix = "");

/// 时间ms
uint64_t GetCurrentMS();
/// 时间us
uint64_t GetCurrentUS();

template<class V, class Map, class K>
V GetParamValue(const Map& m, const K& k, const V& def = V()) {
    auto it = m.find(k);
    if(it == m.end()) {
        return def;
    }
    try {
        return boost::lexical_cast<V>(it->second);
    } catch (...) {
    }
    return def;
}

template<class V, class Map, class K>
bool CheckGetParamValue(const Map& m, const K& k, V& v) {
    auto it = m.find(k);
    if(it == m.end()) {
        return false;
    }
    try {
        v = boost::lexical_cast<V>(it->second);
        return true;
    } catch (...) {
    }
    return false;
}


class StringUtil {
public:
    static std::string Format(const char* fmt, ...);
    static std::string Formatv(const char* fmt, va_list ap);

    static std::string UrlEncode(const std::string& str, bool space_as_plus = true);
    static std::string UrlDecode(const std::string& str, bool space_as_plus = true);

    static std::string Trim(const std::string& str, const std::string& delimit = " \t\r\n");
    static std::string TrimLeft(const std::string& str, const std::string& delimit = " \t\r\n");
    static std::string TrimRight(const std::string& str, const std::string& delimit = " \t\r\n");


    static std::string WStringToString(const std::wstring& ws);
    static std::wstring StringToWString(const std::string& s);

};

template<class T>
void nop(T*) {}

template<class T>
void delete_array(T* v) {
    if(v) {
        delete[] v;
    }
}


std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

class TypeUtil {
public:
    static int8_t ToChar(const std::string& str);
    static int64_t Atoi(const std::string& str);
    static double Atof(const std::string& str);
    static int8_t ToChar(const char* str);
    static int64_t Atoi(const char* str);
    static double Atof(const char* str);
};


}  // namespace sylar

#endif  // __SYLAR_UTIL_H__