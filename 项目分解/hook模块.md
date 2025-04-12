## hook模块
- hook实际上就是对系统调用API进行一次封装，将其封装成一个与原始的系统调用API同名的接口，应用在调用这个接口时，会先执行封装中的操作，再执行原始的系统调用API。hook技术可以使应用程序在执行系统调用之前进行一些隐藏的操作，比如可以对系统提供malloc()和free()进行hook, 在真正进行内存分配和释放之前，统计内存的引用计数，以排查内存泄露问题。
- 还可以用C++的子类重载来理解hook。在C++中，子类在重载父类的同名方法时，一种常见的实现方式是子类先完成自己的操作，再调用父类的操作，如下：
```cpp
class Base {
public:
    void Print() {
        cout << "This is Base" << endl;
    }
};
 
class Child : public Base {
public:
    /// 子类重载时先实现自己的操作，再调用父类的操作
    void Print() {
        cout << "This is Child" << endl;
        Base::Print();
    }
};
```
- 在上面的代码实现中，调用子类的Print方法，会先执行子类的语句，然后再调用父类的Print方法，这就相当于子类hook了父类的Print方法。由于hook之后的系统调用与原始的系统系统调用同名，所以对于程序开发者来说也很方便，不需要重新学习新的接口，只需要按老的接口调用惯例直接写代码就行了。

### hook的功能
- 1. hook的目的是在不重新编写代码的情况下，把老代码中的socket IO相关的API都转成异步，以提高性能。hook和IO协程调度是密切相关的，如果不使用IO协程调度器，那hook没有任何意义，考虑IOManager要在一个线程上按顺序调度以下协程： 
  - 1. 协程1：sleep(2) 睡眠两秒后返回。
  - 2. 协程2：在scoket fd1 上send 100k数据。 
  - 3. 协程3：在socket fd2 上recv直到数据接收成功。
- 2. 在未hook的情况下，IOManager要调度上面的协程，流程是下面这样的：
  - 1. 调度协程1，协程阻塞在sleep上，等2秒后返回，这两秒内调度线程是被协程1占用的，其他协程无法在当前线程上调度。
  - 2. 调度协程2，协程阻塞send 100k数据上，这个操作一般问题不大，因为send数据无论如何都要占用时间，但如果fd迟迟不可写，那send会阻塞直到套接字可写，同样，在阻塞期间，其他协程也无法在当前线程上调度。
  - 3. 调度协程3，协程阻塞在recv上，这个操作要直到recv超时或是有数据时才返回，期间调度器也无法调度其他协程。
- 3. 上面的调度流程最终总结起来就是，协程只能按顺序调度，一旦有一个协程阻塞住了，那整个调度线程也就阻塞住了，其他的协程都无法在当前线程上执行。
  - 1. 像这种一条路走到黑的方式其实并不是完全不可避免，以sleep为例，调度器完全可以在检测到协程sleep后，将协程yield以让出执行权，同时设置一个定时器，2秒后再将协程重新resume。这样，调度器就可以在这2秒期间调度其他的任务，同时还可以顺利的实现sleep 2秒后再继续执行协程的效果
  - 2. send/recv与此类似。在完全实现hook后，IOManager的执行流程将变成下面的方式：
    - 1. 调度协程1，检测到协程sleep，那么先添加一个2秒的定时器，定时器回调函数是在调度器上继续调度本协程，接着协程yield，等定时器超时。
    - 2. 因为上一步协程1已经yield了，所以协徎2并不需要等2秒后才可以执行，而是立刻可以执行。同样，调度器检测到协程send，由于不知道fd是不是马上可写，所以先在IOManager上给fd注册一个写事件，回调函数是让当前协程resume并执行实际的send操作，然后当前协程yield，等可写事件发生。
    - 3. 上一步协徎2也yield了，可以马上调度协程3。协程3与协程2类似，也是给fd注册一个读事件，回调函数是让当前协程resume并继续recv，然后本协程yield，等事件发生。
    - 4. 等2秒超时后，执行定时器回调函数，将协程1 resume以便继续执行。
    - 5. 等协程2的fd可写，一旦可写，调用写事件回调函数将协程2 resume以便继续执行send。
    - 6. 等协程3的fd可读，一旦可读，调用回调函数将协程3 resume以便继续执行recv。
  - 3. 上面的4、5、6步都是异步的，调度线程并不会阻塞，IOManager仍然可以调度其他的任务，只在相关的事件发生后，再继续执行对应的任务即可。并且，由于hook的函数签名与原函数一样，所以对调用方也很方便，只需要以同步的方式编写代码，实现的效果却是异步执行的，效率很高。
- 在IO协程调度中对相关的系统调用进行hook，可以让调度线程尽可能得把时间片都花在有意义的操作上，而不是浪费在阻塞等待中。
###### hook的重点：在替换API的底层实现的同时完全模拟其原本的行为，因为调用方是不知道hook的细节的，在调用被hook的API时，如果其行为与原本的行为不一致，就会给调用方造成困惑。比如，所有的socket fd在进行IO调度时都会被设置成NONBLOCK模式，如果用户未显式地对fd设置NONBLOCK，那就要处理好fcntl，不要对用户暴露fd已经是NONBLOCK的事实，这点也说明，除了IO相关的函数要进行hook外，对fcntl, setsockopt之类的功能函数也要进行hook，才能保证API的一致性。

##### hook的实现基础
- hook的实现机制非常简单，就是通过动态库的全局符号介入功能，用自定义的接口来替换掉同名的系统调用接口。由于系统调用接口基本上是由C标准函数库libc提供的，所以这里要做的事情就是用自定义的动态库来覆盖掉libc中的同名符号。
- 由于动态库的全局符号介入问题，全局符号表只会记录第一次识别到的符号，后续的同名符号都被忽略，但这并不表示同名符号所在的动态库完全不会加载，因为有可能其他的符号会用到。以libc库举例，如果用户在链接libc库之前链接了一个指定的库，并且在这个库里实现了read/write接口，那么在程序运行时，程序调用的read/write接口就是指定库里的，而不是libc库里的。libc库仍然会被加载，因为libc库是程序的运行时库，程序不可能不依赖libc里的其他接口。因为libc库也被加载了，所以，通过一定的手段，仍然可以从libc中拿到属于libc的read/write接口，这就为hook创建了条件。程序可以定义自己的read/write接口，在接口内部先实现一些相关的操作，然后再调用libc里的read/write接口。而将libc库中的接口重新找回来的方法就是使用dlsym()。

#### hook的一些api
- sylar的hook功能是以线程为单位的，可以自由设置当前线程是否使用hook。默认情况下，协程调度器的调度线程会开启hook，而其他线程则不会开启。对以下和函数进行了hook，并且只对socket fd进行了hook，如果操作的不是socket fd，那会直接调用系统原本的API，而不是hook之后的API：

```cpp
sleep
usleep
nanosleep
socket
connect
accept
read
readv
recv
recvfrom
recvmsg
write
writev
send
sendto
sendmsg
close
fcntl
ioctl
getsockopt
setsockopt
```
- 除此外，sylar还增加了一个connect_with_timeout接口用于实现带超时的connect。为了管理所有的socket fd，sylar设计了一个FdManager类来记录所有分配过的fd的上下文，这是一个单例类，每个socket fd上下文记录了当前fd的读写超时，是否设置非阻塞等信息。
- 关于hook模块和IO协程调度的整合。一共有三类接口需要hook，如下：
- 1. sleep延时系列接口，包括sleep/usleep/nanosleep。对于这些接口的hook，只需要给IO协程调度器注册一个定时事件，在定时事件触发后再继续执行当前协程即可。当前协程在注册完定时事件后即可yield让出执行权。
- 2. socket IO系列接口，包括read/write/recv/send...等，connect及accept也可以归到这类接口中。这类接口的hook首先需要判断操作的fd是否是socket fd，以及用户是否显式地对该fd设置过非阻塞模式，如果不是socket fd或是用户显式设置过非阻塞模式，那么就不需要hook了，直接调用操作系统的IO接口即可。如果需要hook，那么首先在IO协程调度器上注册对应的读写事件，等事件发生后再继续执行当前协程。当前协程在注册完IO事件即可yield让出执行权。
- 3. socket/fcntl/ioctl/close等接口，这类接口主要处理的是边缘情况，比如分配fd上下文，处理超时及用户显式设置非阻塞问题。

##### 模块实现
###### 2.1 FdCtx类 ： fd_manager.h
- FdCtx存储每一个fd相关的信息，并由FdManager管理每一个FdCtx，FdManager为单例类。
- 成员变量如下：
- 
```cpp
/// 是否初始化
  bool m_isInit : 1;
  /// 是否socket
  bool m_isSocket : 1;
  /// 是否hook非阻塞
  bool m_sysNonblock : 1;
  /// 是否用户主动设置非阻塞
  bool m_userNonblock : 1;
  /// 是否关闭
  bool m_isClsed : 1;
  /// 文件句柄
  int m_fd;
  /// 读超时时间毫秒
  uint64_t m_recvTimeout;
  /// 写超时时间毫秒
  uint64_t m_sendTimeout;
```
- 获取接口原始地址：
```cpp
#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {
    static bool is_inited = false;
    if(is_inited) {
        return;
    }
// dlsym - 从一个动态链接库或者可执行文件中获取到符号地址。成功返回跟name关联的地址
// RTLD_NEXT 返回第一个匹配到的 "name" 的函数地址
// 取出原函数，赋值给新函数
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX);
#undef XX
}

// 声明变量
extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX
}
```
- extern int a; : 显式的说明了a的存储空间是在程序的其他地方分配的，在文件中其他位置或者其他文件中寻找a这个变量。
- 宏定义展开如下：
- 这一段相当于将linux下的api封装进了原始api函数名+_f的函数指针中。
```cpp
extern "C" {
	sleep_fun sleep_f = nullptr;
    usleep_fun usleep_f = nullptr;
	.....
	setsocketopt_fun setsocket_f = nullptr;
}

void hook_init() {
    static bool is_inited = false;
    if (is_inited) {
        return;
    }
    
	sleep_f = (sleep_fun)dlsym(RTLD_NEXT, "sleep");
    usleep_f = (usleep_fun)dlsym(RTLD_NEXT, "usleep");
    ...
    setsocketopt_f = (setsocketopt_fun)dlsym(RTLD_NEXT, "setsocketopt");
}
```

- set_hook_enable() ： 设置是否hook，定义线程局部变量，来控制是否开启hook
- is_hook_enable()： 获取是否hook。

###### do_io()
- IO操作，hook的核心函数。需要注意的是，这段代码使用了模板和可变参数，可以适用于不同类型的IO操作，能够以写同步的方式实现异步的效果。该函数的主要思想如下：
  -  1. 先进行一系列判断，是否按原函数执行。 
  -  2. 执行原始函数进行操作，若errno = EINTR，则为系统中断，应该不断重新尝试操作。 
  -  3. 若errno = EAGIN，系统已经隐式的将socket设置为非阻塞模式，此时资源咱不可用。
  -  4. 若设置了超时时间，则设置一个执行周期为超时时间的条件定时器，它保证若在超时之前数据就已经来了，然后操作完do_io执行完毕，智能指针tinfo已经销毁了，但是定时器还在，此时弱指针拿到的就是个空指针，将不会执行定时器的回调函数。
  -  5. 在条件定时器的回调函数中设置错误为ETIMEDOUT超时，并且使用cancelEvent强制执行该任务，继续回到该协程执行。
  -  6. 通过addEvent添加事件，若添加事件失败，则将条件定时器删除并返回错误。成功则让出协程执行权。
  -  7. 只有两种情况协程会被拉起： - 超时了，通过定时器回调函数 cancelEvent ---> triggerEvent 会唤醒回来 - addEvent数据回来了会唤醒回来 
  -  8. 将定时器取消，若为超时则返回-1并设置errno = ETIMEDOUT，并返回-1。 
  -  9. 若为数据来了则retry，重新操作。


###### 总结部分：有了hook模块的加持，在使用IO协程调度器时，如果不想该操作导致整个线程的阻塞，我们可以使用scheduler将该任务加入到任务队列中，这样当任务阻塞时，只会使执行该任务的协程挂起，去执行别的任务，在消息来后或者达到超时时间继续执行该协程任务，这样就实现了异步操作。




