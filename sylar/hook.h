#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <cstdint>

namespace sylar {
bool is_hook_enable();

void set_hook_enable(bool flag);
}  // namespace sylar

extern "C" {
/// sleep相关的

// sleep_fun 为函数指针
typedef unsigned int (*sleep_fun)(unsigned int seconds);
// 它是一个sleep_fun类型的函数指针变量，表示该变量在其他文件中已经定义，我们只是在当前文件中引用它。
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec* req, struct timespec* rem);
extern nanosleep_fun nanosleep_f;

/// sock相关的
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

/// 客户端连接服务器用connect
typedef int (*connect_fun)(int socket, const struct sockaddr* address,
                           socklen_t address_len);
extern connect_fun connect_f;

/// 服务器端与客户端建立连接
typedef int (*accept_fun)(int socket, struct sockaddr* address,
                          socklen_t* address_len);
extern accept_fun accept_f;

/// 从文件描述符中读取数据到缓冲区
typedef ssize_t (*read_fun)(int fd, void* buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fildes, const struct iovec* iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int socket, void* buffer, size_t length, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int socket, void* buffer, size_t length,
                                int flags, struct sockaddr* address,
                                socklen_t* address_len);
extern recvfrom_fun recvfrom_f;

typedef ssize_t (*recvmsg_fun)(int socket, struct msghdr* message, int flags);
extern recvmsg_fun recvmsg_f;

/// socked 写相关的函数
typedef ssize_t (*write_fun)(int fd, const void* buf, size_t count);
extern write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec* iov, int iovcnt);
extern writev_fun writev_f;

typedef ssize_t (*send_fun)(int socket, const void* buffer, size_t length,
                            int flags);
extern send_fun send_f;

typedef ssize_t (*sendto_fun)(int socket, const void* message, size_t length,
                              int flags, const struct sockaddr* dest_addr,
                              socklen_t dest_len);
extern sendto_fun sendto_f;

typedef ssize_t (*sendmsg_fun)(int socket, const struct msghdr* message,
                               int flags);
extern sendmsg_fun sendmsg_f;

typedef int (*close_fun)(int fd);
extern close_fun close_f;

///
typedef int (*fcntl_fun)(int fildes, int cmd, ...);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int fildes, unsigned long int request, ... /* arg */);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int socket, int level, int option_name,
                              void* option_value, socklen_t* option_len);
extern getsockopt_fun getsockopt_f;

typedef int (*setsockopt_fun)(int socket, int level, int option_name,
                              const void* option_value, socklen_t option_len);
extern setsockopt_fun setsockopt_f;

extern int connect_with_timeout(int fd, const struct sockaddr* addr,
                                socklen_t addrlen, uint64_t timeout_ms);
}
#endif