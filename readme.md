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
-- 线程模块03:日志模块整合
-- 线程模块04:日志模块整合02
-- 线程模块05:配置模块整合

-- 协程模块01
-- 协程模块02
-- 协程模块03
-- 协程模块04

-- 协程调度模块01
-- 协程调度模块02
-- 协程调度模块03
-- 协程调度模块04
-- 协程调度模块05
-- 协程调度模块06

-- IO协程调度器01
-- IO协程调度器02
-- IO协程调度器03
-- IO协程调度器04


-- IO协程调度器05：定时器
-- IO协程调度器06：定时器
-- IO协程调度器07：定时器


-- Socket IO Hook01
-- Socket IO Hook02
-- Socket IO Hook03
-- Socket IO Hook04
-- Socket IO Hook05
-- Socket IO Hook06

-- 网络模块socket01
-- 网络模块socket02
-- 网络模块socket03
-- 网络模块socket04
-- 网络模块socket05
-- 网络模块socket06：sock封装
-- 网络模块socket07
-- 网络模块socket08

-- 序列化ByteArray01
-- 序列化ByteArray02
-- 序列化ByteArray03
-- 序列化ByteArray04

-- HTTP协议封装01
-- HTTP协议封装02
-- HTTP协议封装03
-- HTTP协议封装04
-- HTTP协议封装05
-- HTTP协议封装06

-- TCPServer封装01 02

-- Stream封装-SocketStream


-- HttpSerlet封装01
-- HttpSerlet封装02
```

## 开发环境

```
manjaro 6.10.13
gcc 14.2.1
cmake 3.30.3
yaml-cpp
boost
pthread
http-parser :https://github.com/nodejs/http-parser
ragel
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
和log来整合
Logger,Appender,

pthread_spinlock_t:用于提升性能
Spinlock,替换Mutex，
写文件，周期性，reopen

```

## 协程库封装 基于linux下的ucontext的

定义协程接口
ucontext_t。
marco

```
Fiber::GetThis()
Thread->main_fiber <------> sub_fiber
            |
            |
            v
        sub_fiber
```

- 协程调度模块scheduel

```
        1 ----  N   1 ----- M
scheduler ---> thread ----> fiber
1.scheduel 线程池，分配一组线程
2.协程调度器，将协程，指定到相应的线程上去执行



N个线程对上m个协程这样的结构，协程可以在线程之间自由切换
m_threads
<function<void()>, fiber, thread_io>  m_fibers

schedule(func/fiber)

start()
stop()
run()

1.设置当前线程的scheduler
2.设置当前的线程的run,fiber
3.协程调度循环while(true)
    1.协程消息队列里面是否有任务
    2.无任务执行，执行idle
    
```

```
IOManager (epoll) ----> Scheduler
    |
    |
    |
    v
    idle (epoll_wait)
    
    信号量
PutMessage(msg,) +信号量1
message_queue
    |
    |----- Thread
    |----- Thread
         wait()-信号量1   RecvMessage(msg,)
         
异步IO，等待数据返回。在epoll_wait等待，没有消息回来会阻塞掉epoll_wait

子线程在idle的时候监控管道的读端，然后epoll_wait阻塞，当需要唤醒子线程的时候向管道的写端写入数据即可，
然后子线程的epoll会调用事件的处理函数，反应堆中调用yield返回调用协程
```

```
Timer -> addTimer() --> cancel()
获取当前的定时器触发距离现在的时间差
返回当前需要触发的定时器
```

```
         [Fiber]
            ^ N                   [Timer]
            |                       ^
            | 1                     |
         [Thread]                   |
            ^ M              [TimerManager]
            |                       ^
            |                       |
            | 1                     |
        [Scheduler] <------ [IOManager(epoll)]
```

## HOOK

    sleep
    usleep
    socket 相关的(socket,connect,accept)
    IO相关的(read,write,send,recv,....)
    fd相关的(fcntl,ioctl,....)

## socket函数库

        [UnixAddress]
             |                           |-[IPv4Address]
        |Address| ---- [IPAddress] ----  |
             |                           |-[IPv6Address]
             | 
         |Socket|

connect,
accept
read/write/close

## 序列化bytearray

write(int, float ,int64, ...)
read(int, float, int64, ...)

##### 结构图

![img.png](img.png)

## http协议开发

HTTP/1.1 - API协议

HttpRequest;
HttpResponse;

GET / HTTP/1.1
Host: www.sylar.top

```
HTTP/1.1 200 OK
Accept-Ranges: bytes
Cache-Control: no-cache
Connection: keep-alive
Content-Length: 29384
Content-Type: text/html

uri : http://www.baid.com/page/xxx?id=10&v=20#fr
    http,协议
    www.baid.com,host
    80 端口
    /page/xxx,path
    id =10&v=20 param
    fr fragment
```

- ![img_1.png](img_1.png)
- 这个是从https://github.com/mongrel2/mongrel2/tree/master/src/http11这个里面得到的
- 并且用ragel生成

## TcpServer封装

基于TcpServer实现了一个EchoServer

## Stream针对文件/socket封装

read/write/readFixSize/writeFixSize

HttpSession/HttpConnection
Server.accept, socket -> session
client connect socket -> Connection

HttpServer : TcpServer

    对于Servlet：交互式的浏览和修改数据，生成动态web内容，狭义Servlet是值java实现的接口，前端页面提出来请求，
        后台如何知道用户想来做什么？并且给这个请求分配对应的处理类
    Servlet <---------- FunctionServlet
        |
        |
        v
    ServletDispatch
    
    