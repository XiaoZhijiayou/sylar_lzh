# sylar

## 课程目录

-- 配置系统05：更多STL容器的支持
-- 配置系统06：自定义类型的支持

## 开发环境

manjaro 6.10.13
gcc 14.2.1
cmake 3.30.3
yaml-cpp
boost

## 项目路径

bin 二进制文件
cmake-build-debug 中间文件
CMakeLists.txt --cmake定义文件
lib 库输出路径
sylar 项目源码路径
test 测试源码路径

## 日志系统

1>
log4J
Logger(定义日志类别)
|
|---------- Formatter(日志格式)
|
Appender(日志输出地方)

## 配置系统

Config ---> Yaml

yamp-cpp:github搜 yay -S yaml-cpp

```cpp
YAML::NODE node = YAML::LoadFile(filename);
node.IsMap();
for(auto it = node.begin();
    it != node.end(); ++it){
        it->first,it->second
}

node.IsSequence()
for(size_t i = 0; i < node.size(); ++i){
    
}

node.IsScalar();//直接输出出来
```

配置系统的原则，约定优于配置：

```cpp
template<T,FromStr,ToStr>
class ConfigVar;

template<F,T>
LexicalCast
//容器的偏特化，目前支持vector
//list,set,map,unordered_set,unorder_map
//map/unordered_map 支持key = std::string
//Config::Lookup(key),key相同，类型不同的，不会报错
```
自定义类型，需要实现sylar::LexicalCast,偏特化
实现后，就可以支持Config解析自定义类型，自定义类型可以和常规stl容器一起使用。


## 协程库封装



    