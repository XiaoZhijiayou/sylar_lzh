# sylar

## 课程目录

```
-- 配置系统05：更多STL容器的支持
-- 配置系统06：自定义类型的支持
-- 配置系统07：配置变更事件
-- 配置系统08：日志系统整合
-- 配置系统09：日志系统整合02
-- 配置系统10:日志系统整合03
-- 配置系统11:日志系统整合04
-- 配置系统12:日志和配置小结部分

-- 线程模块01
-- 线程模块02:信号量和互斥量
```



## 开发环境
```
manjaro 6.10.13
gcc 14.2.1
cmake 3.30.3
yaml-cpp
boost
```

## 项目路径

bin 二进制文件
cmake-build-debug 中间文件
CMakeLists.txt --cmake定义文件
lib 库输出路径
sylar 项目源码路径
test 测试源码路径

## 日志系统

```
1>
log4J
Logger(定义日志类别)
|
|---------- Formatter(日志格式)
|
Appender(日志输出地方)
```

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


配置的事件机制
--- 当一个配置项发生修改的时候，可以反向通知对应的代码，回调
# 日志系统整合配置系统
```
logs:
    - name:root
      level: (debug,info,warn,error,fatal)
      formatter: '%d%T%p%T%t%m%n'
      appender:
            - type:StdoutLogAppender,FileLogAppender
              level:(debug,.....)
              file:/logs/xxx.log
```

```cpp
    sylar::Logger g_logger = 
            sylar::LoggerMgr::GetInstance()->getLogger(name);
            SYLAR_LOG_INFO(g_logger) << "xxx.log";
```

```cpp
static Logger::ptr g_log = SYLAR_LOG_NAME("system");
//m_root,m_system-m_root 当logger的appenders为空，使用root写logger
```
```cpp
//定义LogDefine LogAppenderDefine,偏特化 LexicalCast
//实现日志配置解析
```

```cpp

```
遗留问题：
1.appender定于的formatter读取yaml的时候，没有被初始化掉
2.去掉额外的调试日志

#### 日志和配置小结部分

## 线程库
```
Thread,Mutex(锁机制)

pthread pthread_create
互斥量 mutex
信号量 semaphore
```


## 协程库封装



    