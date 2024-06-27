#include <stdarg.h>

#include"hook.h"
#include"iomanager.h"
#include "util.h"

//日志调试
static auto logger=GET_LOGGER("system");
namespace HSQ
{
//**********************************************************************************

#define FUN_NAME(fun) (#fun)


//线程局部变量，标记当前线程是否hook
static thread_local bool t_local_hook_flag=false;
static uint64_t s_connect_timeout = -1;
bool IsHook()      //返回线程局部变量标记
{
    return t_local_hook_flag;
}
void SetHook(bool flag)     //设置线程局部变量标记
{
    t_local_hook_flag=flag;
}

#define HOOK_FUN(REPLACE) \
    REPLACE(sleep) \
    REPLACE(usleep) \
    REPLACE(nanosleep) \
    REPLACE(socket) \
    REPLACE(connect) \
    REPLACE(accept) \
    REPLACE(read) \
    REPLACE(readv) \
    REPLACE(recv) \
    REPLACE(recvfrom) \
    REPLACE(recvmsg) \
    REPLACE(write) \
    REPLACE(writev) \
    REPLACE(send) \
    REPLACE(sendto) \
    REPLACE(sendmsg) \
    REPLACE(close) \
    REPLACE(fcntl) \
    REPLACE(ioctl) \
    REPLACE(getsockopt) \
    REPLACE(setsockopt)

void hook_init()
{
    static bool is_inited = false;
    if(is_inited) 
        return;
#define REPLACE(name) name ## _sys= (name ## _self)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(REPLACE);
#undef REPLACE
}

struct Hook_Init
{
    Hook_Init()
    {
        hook_init();
        s_connect_timeout=5000;
    }
};
static Hook_Init Init;

//**********************************************************************************
//namespace end
}

//IO处理逻辑
struct timed_task_info
{
    bool is_canceled=false;
};
template<typename SysFunction,typename...Args>
static ssize_t io_process(int fd,SysFunction fun,const char* hook_name,HSQ::IOManager::IOEventType event_type,
                        int timeout_type,Args&&...args)
{
    //如果当前线程没有设置hook标记，则使用系统调用处理
    if(!HSQ::t_local_hook_flag)
        return fun(fd, std::forward<Args>(args)...);
    //根据fd，检查是否在所管理的fd集合中
    //DEBUG(logger, "在io_process准备调用GetFd");
    HSQ::Fd::ptr fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(fd);
    //DEBUG(logger, "在io_process调用GetFd处理完毕");

    //如果不在所管理的集合中，则由系统调用处理
    if(!fd_ptr)
    {
        ASSERT(fun!=nullptr,"fun==nullptr");
        return fun(fd,std::forward<Args>(args)...);
    }
       
    //如果fd已经关闭，则读写出错
    if(fd_ptr->IsClose())
    {
        errno=EBADF;
        DEBUG(logger, "fd_ptr is close");
        return -1;
    }
    //判断是否为socket文件描述符，对于非socket文件描述符，交给系统调用处理
    //如果是socket文件描述符，但是用户已经设置为非阻塞，表明用户打算自己处理
    //非阻塞文件描述符返回EAGIN的情况，同样交给系统调用处理
    if(!fd_ptr->IsSocket() || fd_ptr->IsUserNonBlock())
        return fun(fd,std::forward<Args>(args)...);
    
    uint64_t time_out=fd_ptr->GetTimeOut(timeout_type);
    //添加条件定时任务时，task_info的weak_ptr将作为一个条件判断使用
    std::shared_ptr<timed_task_info> task_info(new timed_task_info);

retry:
    //调用系统函数
    ssize_t read_bytes=fun(fd,std::forward<Args>(args)...);
    //系统调用返回，如果是因为系统中断，继续执行
    while(read_bytes==-1 && errno==EINTR)
        read_bytes=fun(fd,std::forward<Args>(args)...);
    
    //否则，如果系统调用因为条件未达成（读缓冲区空，写缓冲区满）
    //DEBUG(logger, "在io_process通过系统调用获取的字节数：" + std::to_string(read_bytes));

    if(read_bytes==-1 && errno==EAGAIN)
    {
        //获取当前调度器,如果fd设置了超时时间,添加定时任务
        HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();

        HSQ::TimedTask::ptr timed_task=nullptr;
        //创建判断条件
        std::weak_ptr<timed_task_info> condition(task_info);

        //如果设置了超时时间，创建添加定时任务
        if(time_out!=(uint64_t)-1)
        {
            auto task=[time_out,condition,iomanager,event_type,fd]()
            {
                auto temp=condition.lock();
                if(!temp || temp->is_canceled)
                    return;
                temp->is_canceled=ETIMEDOUT;
                iomanager->CancelIOEvent(fd,event_type);
            };
            timed_task=iomanager->AddTimedTask(std::move(task),time_out,condition);
        }

        //添加IO事件，注册到epoll，在超时时间内，如果条件达成，通过epoll_wait返回事件，则取消定时任务
        //DEBUG(logger, "在io_process调用AddIOEvent注册fd");
        int rt=iomanager->AddIOEvent(fd,event_type,nullptr);
        //DEBUG(logger, "在io_process调用AddIOEvent注册fd处理完毕");

        if(rt==-1)
        {
            ERROR(logger,"io_process : iomanager->AddIOEvent error");
            if(timed_task)
                timed_task->CancelTimedTask();
            return -1;
        }
        else
        {   //当前协程唤醒时，有两种情况：
            //1--:超时时间内，从epoll_wait得到唤醒，则条件达成（读缓冲区可读，或写缓冲区可写），此时若有定时任务，则取消
            //2--：超时时间内条件没有达成，最终通过定时任务唤醒，则检查errno，返回-1
            //DEBUG(logger, "在io_process调用YieldToHold");

            HSQ::Fiber::GetCurrentRunningFiber()->YieldToHold();
            //DEBUG(logger, "在io_process调用YieldToHold处理完毕");

            if(timed_task)
                timed_task->CancelTimedTask();
            if(task_info->is_canceled)
            {
                errno=task_info->is_canceled;
                return -1;
            }
        }
        goto retry;
    }
    return read_bytes;
}

extern "C"
{
//实现本地函数，取代系统调用
//xx_self:自己定义的函数   xx_sys：系统调用
#define REPLACE(name)  name ## _self name ## _sys =nullptr;
    HOOK_FUN(REPLACE);
#undef REPLACE
//sleep
unsigned int sleep(unsigned int seconds)
{
    //如果当前线程没有设置hook标记，则使用原来的系统调用即可
    if(HSQ::t_local_hook_flag==false)
        return sleep_sys(seconds);
    //slepp的处理过程：添加定时任务，时长为seconds，绑定的任务为调度当前协程
    HSQ::Fiber::ptr fiber=HSQ::Fiber::GetCurrentRunningFiber();
    HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();
    //知识点：std::bind绑定模板函数，需要提供模板类型
    auto task=[iomanager, fiber](){iomanager->Schedule(fiber);};
    iomanager->AddTimedTask(task,seconds*1000);
    // iomanager->AddTimedTask([iomanager,&fiber]()
    // {iomanager->Schedule(std::move(fiber));},seconds*1000);

    HSQ::Fiber::YieldToHold();
    return 0;
}


int usleep(useconds_t usec)
{
    if(HSQ::t_local_hook_flag==false)
        return usleep_sys(usec);
    
    HSQ::Fiber::ptr fiber=HSQ::Fiber::GetCurrentRunningFiber();
    HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();

    auto task=[iomanager,&fiber](){iomanager->Schedule(std::move(fiber));};
    iomanager->AddTimedTask(std::move(task),usec/1000);
    HSQ::Fiber::YieldToHold();
    return 0;
}


int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if(HSQ::t_local_hook_flag==false)
        return nanosleep_sys(req,rem);
    
    HSQ::Fiber::ptr fiber=HSQ::Fiber::GetCurrentRunningFiber();
    HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();

    auto timeout=req->tv_sec*1000+req->tv_nsec/1000/1000;
    auto task=[iomanager,&fiber]{iomanager->Schedule(std::move(fiber));};

    iomanager->AddTimedTask(std::move(task),timeout);
    HSQ::Fiber::YieldToHold();
    return 0;
}

//socket
int socket(int domain, int type, int protocol)
{
    if(!HSQ::t_local_hook_flag)
        return socket_sys(domain,type,protocol);
    int fd=socket_sys(domain,type,protocol);
    if(fd==-1)
        return fd;
    //GetFd(fd,true)已经将fd设置为非阻塞
    HSQ::SingleFdManager::GetInstance()->GetFd(fd,true);
    return fd;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
   return connect_with_timeout(sockfd, addr, addrlen, HSQ::s_connect_timeout);
}


int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{    
    DEBUG(logger, "在全局accept中准备调用io_process");
    int fd=io_process(s,accept_sys,"accept",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,addr,addrlen);
    DEBUG(logger, "在全局accept中调用io_process处理完毕,获得的fd是" + std::to_string(fd));

    //设置为non-block
    if(fd>=0) {
        DEBUG(logger, "在全局accept中准备调用GetFd");
        auto res = HSQ::SingleFdManager::GetInstance()->GetFd(fd,true);
        DEBUG(logger, "在全局accept中调用GetFd，新创建的fd = " + std::to_string(fd));
    }
    return fd;
}


//read
ssize_t read(int fd, void *buf, size_t count)
{
    return io_process(fd,read_sys,"read",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,buf,count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    return io_process(fd,readv_sys,"readv",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,iov,iovcnt);
}


ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    return io_process(sockfd,recv_sys,"recv",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,buf,len,flags);
}


ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen)
{
    return io_process(sockfd,recvfrom_sys,"recvfrom",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
}


ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    return io_process(sockfd,recvmsg_sys,"recvmsg",HSQ::IOManager::IOEventType::READ,SO_RCVTIMEO,msg,flags);
}

//write
ssize_t write(int fd, const void *buf, size_t count)
{
  
   return io_process(fd,write_sys,"write",HSQ::IOManager::IOEventType::WRITE,SO_SNDTIMEO,buf,count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    return io_process(fd,writev_sys,"writev",HSQ::IOManager::IOEventType::WRITE,SO_SNDTIMEO,iov,iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags)
{
    return io_process(s,send_sys,"send",HSQ::IOManager::IOEventType::WRITE,SO_SNDTIMEO,msg,len,flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen)
{
    return io_process(s,sendto_sys,"sendto",HSQ::IOManager::IOEventType::WRITE,SO_SNDTIMEO,msg,len,flags,to,tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
    return io_process(s,sendmsg_sys,"sendmsg",HSQ::IOManager::IOEventType::WRITE,SO_SNDTIMEO,msg,flags);
}


/*关闭文件描述符的逻辑
*1---当前线程是否设置hook标记，否---使用系统调用关闭描述符
*2---设置了hook标记，检查是否在管理的fd集合中，若在集合中，通过iomanager取消
*    该文件描述符上所有监听的事件，从epoll注册表中删除。将该描述符从管理集合中移除
*3---使用系统调用关闭fd
***/
int close(int fd)
{
    if(HSQ::t_local_hook_flag==false)
        return close_sys(fd);
    
    HSQ::Fd::ptr fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(fd);
    if(fd_ptr!=nullptr)
    {
        HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();
        if(iomanager!=nullptr)
            iomanager->CancelAllIOEvent(fd);
        HSQ::SingleFdManager::GetInstance()->DeleteFd(fd);
    } 
    return close_sys(fd);
}

//
int fcntl(int fd, int cmd, ... /* arg */ )
{
    va_list va;
    va_start(va,cmd);
    switch(cmd)
    {
        //设置文件描述符状态
        case F_SETFL:
                    {
                        int arg=va_arg(va,int);
                        va_end(va);
                        HSQ::Fd::ptr fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(fd);
                        if(!fd_ptr || fd_ptr->IsClose() || !fd_ptr->IsSocket())
                            return fcntl_sys(fd,cmd,arg);
                        //只有在fd集合中的socket设置了SysNonBlock
                        fd_ptr->SetUserNonBlock(arg & O_NONBLOCK);
                        if(fd_ptr->IsSysNonBlock())
                            arg |=O_NONBLOCK;
                        else
                            arg &= ~O_NONBLOCK;
                        return fcntl_sys(fd,cmd,arg);
                    }
                    break;
        //获取文件描述符状态
        case F_GETFL:
                    {
                        va_end(va);
                        int arg=fcntl_sys(fd,cmd);
                        auto fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(fd);
                        if(!fd_ptr || fd_ptr->IsClose() || !fd_ptr->IsSocket())
                        {
                            return arg;
                        }
                        if(fd_ptr->IsUserNonBlock())
                            return arg | O_NONBLOCK;
                        else
                            return arg & ~O_NONBLOCK;
                    }
                    break;
        case F_DUPFD:           //拷贝文件描述符
        case F_DUPFD_CLOEXEC:   //复制文件描述符，同时close_on_exec
        case F_SETFD:           //设置文件描述符标记
        //信号管理
        case F_SETOWN:  //设置文件描述符fd上的事件发送到arg中给定的ID。
                        //进程ID，进程组ID,进程ID指定为正值;进程组ID为
                        //为负值。最常见的是调用过程
                        //将自身指定为所有者(也就是说，arg被指定为getpid(2))        
        case F_SETSIG:  //当可读或可写时，返回该信号
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
                    {
                        int arg = va_arg(va, int);
                        va_end(va);
                        return fcntl_sys(fd, cmd, arg); 
                    }
                    break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
                    {
                        va_end(va);
                        return fcntl_sys(fd, cmd);
                    }
                    break;
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
                    {
                        struct flock* arg = va_arg(va, struct flock*);
                        va_end(va);
                        return fcntl_sys(fd, cmd, arg);
                    }
                    break;
        case F_GETOWN_EX:
        case F_SETOWN_EX:
                    {
                        struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                        va_end(va);
                        return fcntl_sys(fd, cmd, arg);
                    }
                    break;
        default:
                    va_end(va);
                    return fcntl_sys(fd, cmd);
    }
}


//ioctl：操作IO设备参数，对于某些特殊文件，如终端
int ioctl(int d, unsigned long int request, ...)
{
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        HSQ::Fd::ptr fd_ptr = HSQ::SingleFdManager::GetInstance()->GetFd(d);
        if(!fd_ptr || fd_ptr->IsClose() || !fd_ptr->IsSocket()) {
            return ioctl_sys(d, request, arg);
        }
        fd_ptr->SetUserNonBlock(user_nonblock);
    }
    return ioctl_sys(d, request, arg);
}
int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    return getsockopt_sys(sockfd,level,optname,optval,optlen);
}
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen)
{
    if(HSQ::t_local_hook_flag==false)
        return setsockopt_sys(sockfd,level,optname,optval,optlen);

    if(level==SOL_SOCKET)
    {
        if(optname==SO_RCVTIMEO || optname==SO_SNDTIMEO)
        {
            HSQ::Fd::ptr fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(sockfd);
            if(fd_ptr)
            {
                const timeval* v=(const timeval*)optval;
                fd_ptr->SetTimeOut(optname,v->tv_sec*1000 + v->tv_usec /1000);
            }
        }
    }
    return setsockopt_sys(sockfd,level,optname,optval,optlen);    
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
{
    if(!HSQ::t_local_hook_flag)
        return connect_sys(fd,addr,addrlen);
    
    HSQ::Fd::ptr fd_ptr=HSQ::SingleFdManager::GetInstance()->GetFd(fd);

    if(!fd_ptr || fd_ptr->IsClose())
    {
        errno=EBADF;
        return -1;
    }
    if(!fd_ptr->IsSocket())
    {
        return connect_sys(fd,addr,addrlen);
    }
    if(fd_ptr->IsUserNonBlock())
        return connect_sys(fd,addr,addrlen);

    int n=connect_sys(fd,addr,addrlen);
    if(n==0)
        return  0;
    else if(n!=-1 || errno !=EINPROGRESS)   //EINPROGRESS，连接还在进行中
        return n;

    HSQ::IOManager* iomanager=HSQ::IOManager::GetIOManager();
    HSQ::TimedTask::ptr timed_task;
    std::shared_ptr<timed_task_info> task_info(new timed_task_info);
    std::weak_ptr<timed_task_info> weak_task_info(task_info);

    if(timeout_ms !=(uint64_t)-1 )
    {
        auto task=[weak_task_info,iomanager,fd]
        {
            auto temp=weak_task_info.lock();
            if(!temp || temp->is_canceled)
                return;
            temp->is_canceled=ETIMEDOUT;
            iomanager->CancelIOEvent(fd,HSQ::IOManager::IOEventType::WRITE);
        };
        timed_task=iomanager->AddTimedTask(std::move(task),timeout_ms,weak_task_info);
    }

    int rt=iomanager->AddIOEvent(fd,HSQ::IOManager::IOEventType::WRITE,nullptr);
    if(rt==0)
    {
        HSQ::Fiber::YieldToHold();
        if(timed_task)
            timed_task->CancelTimedTask();
        if(task_info->is_canceled)
        {
            errno=task_info->is_canceled;
            return -1;
        }
        else
        {
            if(timed_task)
                timed_task->CancelTimedTask();
            ERROR(logger,("connect error fd="+std::to_string(fd)));
        }
    }
    int error=0;
    socklen_t len=sizeof(int);
    if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&len)==-1)
        return -1;
    if(!errno)
        return 0;
    else
    {
        errno=error;
        return -1;
    }
}


}





