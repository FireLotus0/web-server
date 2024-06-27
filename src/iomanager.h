#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>  //memset
#include "util.h"
#include <fcntl.h>

#include "timer.h"
#include "scheduler.h"
 
#include "macro.h"
#include "fiber.h"
 
/*
*IO调度模块属于线程池模型的同步服务层，基于epoll对IO事件进行封装，并添加到同步队列中，交由协程调度器调度处理
*class IOManager是一个单例对象
*/

namespace HSQ
{
//***********************************************************

class IOManager:public HSQ::Timer,public HSQ::Scheduler
{
public:
    typedef RWMutex RWMutexType;

    using ptr=std::shared_ptr<IOManager>;
    //IO事件类型
    enum IOEventType
    {
        NONE=0x0,
        READ=0x1,
        WRITE=0x4
    };

    //IO事件
    struct IOEvent
    {
        //事件内容
        struct EventContent
        {
            Scheduler* scheduler=nullptr;
            Fiber::ptr fiber=nullptr;
            
            std::function<void()> task;
        };
        std::mutex ioevent_mt;
        //读写事件
        EventContent read_event;
        EventContent write_event;
        //IO句柄
        int fd=0;
        IOEventType ioevent_types=NONE;
        //根据事件类型获取相应的事件内容：read，write
        EventContent& GetEventContent(IOEventType type);
        void ResetEventContent(EventContent& ct);
        //根据事件类型，触发事件
        void TriggerIOEvent(IOEventType type);
    };

    static IOManager* GetThis();


public:
    //参数用于初始化父类
    IOManager(uint32_t threads=1,bool use_caller=true,const std::string name="IOManager");

    ~IOManager();

    //添加IO事件，注册到epoll.成功返回0，失败返回-1
    int AddIOEvent(int fd,IOEventType type,std::function<void()> cb);

    //删除IO事件
    bool DeleteIOEvent(int fd,IOEventType type);

    //取消事件，如果事件存在，触发事件
    bool CancelIOEvent(int fd,IOEventType type);

    //取消所有事件
    bool CancelAllIOEvent(int fd);

    static IOManager* GetIOManager();

protected://继承自父类的虚函数实现
        //向epoll_wait中写入数据，唤醒等待epoll_wait的线程
    void Tickle() override;

    //当线程空闲时，应当执行此方法
    void Idle()override;

    //返回是否可以停止：当前任务处理完成才停止
    bool Stopping()override;
    //定时任务被插入到小顶堆堆顶，触发此事件
    void InsertAtFront()override;

    //扩容策略，设置成protected，以后修改策略比较方便
    void IOEventVecResize(size_t size);

    //重载函数：判断是否可以停止，传入最近一个定时任务
    //距离当前时刻的毫秒数
    bool stopping(uint64_t& timeout);
private:

    //epoll句柄
    int ep_fd=0;
    //pipe管道，将读端注册到epoll，通过写端可以实现
    //手动唤醒epoll_wait的功能
    int pipe_fds[2];

    //当前待处理事件数量
    std::atomic<size_t> pending_event_count{0};
    //IO事件容器
    std::vector<IOEvent*> ioevents_vec;

    RWMutexType iomanager_mt;
};

//***********************************************************
//namespace end 
}