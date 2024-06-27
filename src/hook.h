#pragma once


/*********************************************************
 * 
 * hook的意义：
 * 对于传统的线程池模型，主线程只需要epoll_wait，将就绪的任务加到同步队列中即可
 * 线程池中的线程则负责从同步队列中取出任务，执行任务即可。当某一个socket系统调用（例如send，recv）阻塞时，整个
 * 线程都会阻塞。在多协程的情况下，如果某一个读写操作发生了阻塞，不应该将整个线程阻塞，否则其他协程将不能得到调度执行，
 * 失去了协程提高并发度的功能。所以。需要hook这些系统调用，当阻塞时，只用将当前协程阻塞就行，当前协程让出cpu，继续调度其他协程。
 * 
 * hook的原理：
 * hook通过替换系统调用函数的入口地址，当执行系统调用时，本质上是执行我们自己写的与系统调用同名函数。
 * dlsym：用于从动态库中找到函数的调用地址：void* dlsym(void* handle,const char* symbol)
 * handle:  RTLD_DEFAULT表示按默认的顺序搜索共享库中符号symbol第一次出现的地址
 *          RTLD_NEXT表示在当前库以后按默认的顺序搜索共享库中符号symbol第一次出现的地址
 * 
 * 实现线程级的hook：通过使用线程句变量，标记是否hook，true---当前线程相关的系统调用都被hook
 * *********************************************************/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>

#include"macro.h"
#include"util.h"
#include"fiber.h"
#include"fdmanager.h"
namespace HSQ
{
//**********************************************************************************
    
bool IsHook(); //返回线程局部变量标记

void SetHook(bool flag);//设置线程局部变量标记

//**********************************************************************************
//namespace end
}
/*********************************************************
 * 
 * c语言中没有函数重载，所以编译器修饰函数名时，直接是啥就是啥
 * 但是c++中具有函数重载，编译器修饰函数名时，会加上函数参数之类的，
 * 有时还会加上当前的作用域，例如：typeid([](){}).name() 
 * extern "C"表明其中的函数使用C风格的函数命名规则，与C兼容
 * ********************************************************/
extern "C"
{

//xx_self:自己定义的函数   xx_sys：系统调用

//sleep
typedef unsigned int (*sleep_self)(unsigned int seconds);
extern sleep_self sleep_sys;

typedef int (*usleep_self)(useconds_t usec);
extern usleep_self usleep_sys;

typedef int (*nanosleep_self)(const struct timespec *req, struct timespec *rem);
extern nanosleep_self nanosleep_sys;


//socket
typedef int (*socket_self)(int domain, int type, int protocol);
extern socket_self socket_sys;

typedef int (*connect_self)(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
extern connect_self connect_sys;

typedef int (*accept_self)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_self accept_sys;

//read
typedef ssize_t (*read_self)(int fd, void *buf, size_t count);
extern read_self read_sys;

typedef ssize_t (*readv_self)(int fd, const struct iovec *iov, int iovcnt);
extern readv_self readv_sys;

typedef ssize_t (*recv_self)(int sockfd, void *buf, size_t len, int flags);
extern recv_self recv_sys;

typedef ssize_t (*recvfrom_self)(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_self recvfrom_sys;

typedef ssize_t (*recvmsg_self)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_self recvmsg_sys;

//write
typedef ssize_t (*write_self)(int fd, const void *buf, size_t count);
extern write_self write_sys;

typedef ssize_t (*writev_self)(int fd, const struct iovec *iov, int iovcnt);
extern writev_self writev_sys;

typedef ssize_t (*send_self)(int s, const void *msg, size_t len, int flags);
extern send_self send_sys;

typedef ssize_t (*sendto_self)(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
extern sendto_self sendto_sys;

typedef ssize_t (*sendmsg_self)(int s, const struct msghdr *msg, int flags);
extern sendmsg_self sendmsg_sys;

typedef int (*close_self)(int fd);
extern close_self close_sys;

//
typedef int (*fcntl_self)(int fd, int cmd, ... /* arg */ );
extern fcntl_self fcntl_sys;

typedef int (*ioctl_self)(int d, unsigned long int request, ...);
extern ioctl_self ioctl_sys;

typedef int (*getsockopt_self)(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
extern getsockopt_self getsockopt_sys;

typedef int (*setsockopt_self)(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
extern setsockopt_self setsockopt_sys;

extern int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);
}

