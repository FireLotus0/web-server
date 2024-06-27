#include"iomanager.h"


namespace HSQ
{
//调试日志
static auto logger=GET_LOGGER("system");
//***********************************************************
//**********************IOManager::IOEvent

//根据事件类型获取相应的事件内容(数据成员)：read，write
IOManager::IOEvent::EventContent& IOManager::IOEvent::GetEventContent(IOEventType type)
{
    switch(type)
    {
        case IOManager::IOEventType::READ: return read_event;
        case IOManager::IOEventType::WRITE: return write_event;
        default:
            ERROR(logger," IOManager::IOEvent::GetEventContent");
    }
    throw std::invalid_argument("IOManager::IOEvent::GetEventContent");
}
void IOManager::IOEvent::ResetEventContent(EventContent& ct)
{
    ct.fiber.reset();
    ct.scheduler=nullptr;
    ct.task=nullptr;
}
//根据事件类型，触发事件
void IOManager::IOEvent::TriggerIOEvent(IOEventType type)
{
    //检查IO事件上确有该类型的事件
    ASSERT(type&ioevent_types,"IOManager::IOEvent::TriggerIOEvent no this type");
    //将该类型事件从监听事件集合去除
    ioevent_types=(IOEventType)(ioevent_types & ~type);
    //取出事件
    EventContent& content=GetEventContent(type);
    if(content.task)
        content.scheduler->Schedule(std::move(content.task));
    else
        content.scheduler->Schedule(std::move(content.fiber));
    content.scheduler=nullptr;
    return;
}


//**********************IOManager
//public:
//参数用于初始化父类
IOManager::IOManager(uint32_t threads,bool use_caller,const std::string name):Scheduler(threads,use_caller,name)
{
    //创建epoll
    ep_fd=epoll_create(100);
    ASSERT(ep_fd>0,"ep_fd <=0 error");

    //创建pipe管道
    int rt=pipe(pipe_fds);
    ASSERT(!rt,"pipe  创建失败");

    epoll_event event;
    memset(&event,0,sizeof(event));
    //监听的读事件，设置为ET模式
    event.events=EPOLLIN|EPOLLET;
    //当事件发生时，epoll_wait返回该值
    event.data.fd=pipe_fds[0];

    //将管道读端设置为非阻塞模式，epoll只支持非阻塞文件描述符
    rt=fcntl(pipe_fds[0],F_SETFL,O_NONBLOCK);
    ASSERT(!rt,"fcntl error");

    //向epoll注册管道的读端
    rt=epoll_ctl(ep_fd,EPOLL_CTL_ADD,pipe_fds[0],&event);
    ASSERT(!rt,"epoll_ctl  error,epoll_ctl(ep_fd,EPOLL_CTL_ADD,pipe_fds[0],&event)")
    //创建IO事件集合，初始化为
    IOEventVecResize(32);
    //启动调度器，创建线程池
    Start();
}

IOManager::~IOManager()
{
    //停止调度器
    Stop();
    //关闭构造函数中打开的文件描述符
    close(ep_fd);
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    //释放事件集合中的元素
    for(auto& v:ioevents_vec)
    {
        //如果不是空指针
        if(v)
            delete v;
    }

}

//添加IO事件，注册到epoll.成功返回0，失败返回-1
int IOManager::AddIOEvent(int fd,IOEventType type,std::function<void()> cb)
{
    IOEvent* ioevent=nullptr;
    DEBUG(logger, "在IOManager中准备获取lock");

    HSQ::WriteLock lock(iomanager_mt);
    DEBUG(logger, "在IOManager中获取lock成功");

    //IOEventVec中以fd作为下标
    if(int(ioevents_vec.size()<=fd))
        IOEventVecResize(1.5*fd);
    ioevent=ioevents_vec[fd];
    DEBUG(logger, "iomanager write unlock");
    lock.unlock();

    ASSERT(ioevent!=nullptr,"ioevent==nullptr");
    std::lock_guard<std::mutex> lock2(ioevent->ioevent_mt);
    //判断操作类型，是修改操作还是添加操作
    //如果ioevent已经有读或者写类型事件，则操作位修改，否则添加
    int op=ioevent->ioevent_types?EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=EPOLLET|ioevent->ioevent_types|type;
    //epoll_wait上该fd触发事件时，返回该指针
    event.data.ptr=ioevent;

    //注册到epoll
    int rt=epoll_ctl(ep_fd,op,fd,&event);
    if(rt)
    {
        ERROR(logger,"IOManager::AddIOEvent int rt=epoll_ctl(ep_fd,op,fd,&event); error");
        return -1;
    }
      
    
    //待处理的事件数量+1
    ++pending_event_count;
    //添加监听的事件类型到该IOEvent中，表明IOEvent具有该类型的事件
    ioevent->ioevent_types=(IOEventType)(ioevent->ioevent_types | type);

    //检查该事件状态是否正常
    IOEvent::EventContent& event_content=ioevent->GetEventContent(type);
    ASSERT(!event_content.fiber && !event_content.scheduler && !event_content.task, \
        "IOManager::AddIOEvent event_content.fiber && event_content.scheduler && event_content.task error");

    //如果cb==nullptr,加入的事件仅仅只是为了阻塞一段时间，然后从epoll_wait唤醒，继续执行。用于socket I/O阻塞情况
    event_content.scheduler=Scheduler::GetScheduler();
    if(cb)
        event_content.task.swap(cb);
    else
    {   //主协程的初始状态必须为EXEC
        event_content.fiber=Fiber::GetCurrentRunningFiber();
        ASSERT(event_content.fiber->GetFiberState()==Fiber::State::EXEC,\
                "state="+HSQ::FiberStateToString(event_content.fiber->GetFiberState()));
    }

    return 0;
}

//删除IO事件
bool IOManager::DeleteIOEvent(int fd,IOEventType type)
{
    HSQ::ReadLock lock(iomanager_mt);
    //检查下标
    if(int(ioevents_vec.size())<=fd)
        return false;

    IOEvent* ioevent=ioevents_vec[fd];
    lock.unlock();
    std::lock_guard<std::mutex> lock2(ioevent->ioevent_mt);
    if(!(ioevent->ioevent_types & type))
        return false;
    //去除该事件
    IOEventType new_event_type=(IOEventType)(ioevent->ioevent_types & ~type);
    //如果去除该事件后，fd上已经没有其他事件，则从epoll删除，否则修改
    int op=new_event_type? EPOLL_CTL_MOD:EPOLL_CTL_DEL;
    epoll_event event;
    event.events=EPOLLET|new_event_type;
    event.data.ptr=ioevent;

    int rt=epoll_ctl(ep_fd,op,fd,&event);
    if(rt)
    {
        ERROR(logger," IOManager::DeleteIOEvent epoll_ctl(ep_fd,op,fd,&event); error");
        return false;
    }
       
    //减少待处理事件数量
    --pending_event_count;
    ioevent->ioevent_types=new_event_type;
    IOEvent::EventContent& event_content=ioevent->GetEventContent(type);
    ioevent->ResetEventContent(event_content);
    return true;
}

//取消事件，如果事件存在，触发事件
bool IOManager::CancelIOEvent(int fd,IOEventType type)
{
    IOEvent* ioevent=nullptr;
    HSQ::ReadLock lock(iomanager_mt);
    if(ioevents_vec.size()<=fd)
        return false;
    ioevent=ioevents_vec[fd];
    lock.unlock();

    std::lock_guard<std::mutex> lock2(ioevent->ioevent_mt);
    if(!(ioevent->ioevent_types & type))
    {
        ERROR(logger,"IOManager::CancelIOEvent ioevent->ioevent_types & type==false" );
        return false;
    }

    IOEventType new_event_type=(IOEventType)(ioevent->ioevent_types & ~type);
    int op=new_event_type?EPOLL_CTL_MOD:EPOLL_CTL_DEL;
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=EPOLLET|new_event_type;
    event.data.ptr=ioevent;

    int rt=epoll_ctl(ep_fd,op,fd,&event);
    if(rt)
    {
        ERROR(logger,"IOManager::CancelIOEvent int rt=epoll_ctl(ep_fd,op,fd,&event) error");
        return false;
    }
    //触发该fd上的相应事件
    ioevent->TriggerIOEvent(type);
    --pending_event_count;
    return true;
}

//取消所有事件
bool IOManager::CancelAllIOEvent(int fd)
{
    IOEvent* ioevent=nullptr;
    HSQ::ReadLock lock(iomanager_mt);
    if((int)ioevents_vec.size()<=fd)
        return false;
    ioevent=ioevents_vec[fd];
    lock.unlock();

    std::lock_guard<std::mutex> lock2(ioevent->ioevent_mt);
    //如果fd上没有事件，返回即可
    if(!ioevent->ioevent_types)
        return false;
    
    int op=EPOLL_CTL_DEL;
    epoll_event event;
    memset(&event,0,sizeof(event));
    event.events=0;
    event.data.ptr=ioevent;

    int rt=epoll_ctl(ep_fd,op,fd,&event);
    if(rt)
    {
        ERROR(logger,"IOManager::CancelAllIOEvent int rt=epoll_ctl(ep_fd,op,fd,&event)");
        return false;
    }

    if(ioevent->ioevent_types & IOEventType::READ)
    {
        ioevent->TriggerIOEvent(IOEventType::READ);
        --pending_event_count;
    }
    if(ioevent->ioevent_types & IOEventType::WRITE)
    {
        ioevent->TriggerIOEvent(IOEventType::WRITE);
        --pending_event_count;
    }
    ASSERT(ioevent->ioevent_types==0,"IOManager::CancelAllIOEvent(int fd) ioevent->ioevent_types==0,");
    return true;
}

//静态成员函数：GetIOManager()
IOManager* IOManager::GetIOManager()
{
    return dynamic_cast<IOManager*>(Scheduler::GetScheduler());
}

//protected:继承自父类的虚函数实现
//向epoll_wait中写入数据，唤醒等待epoll_wait的线程
void IOManager::Tickle()
{
    //先看一下是否有空闲线程，如果没有，说明没有线程阻塞在epoll_wait
    //即使唤醒，也没有作用
    if(!HasIdleThread())
        return;
    int rt=write(pipe_fds[1],"T",1);
    ASSERT(rt==1,"IOManager::Tickle() int rt=write(pipe_fds[1],\"T\",1); error");
}


//返回是否可以停止：当前任务处理完成才停止
bool IOManager::Stopping()
{
    uint64_t timeout=0;
    return stopping(timeout);
}
//定时任务被插入到小顶堆堆顶，触发此事件
void IOManager::InsertAtFront()
{
    Tickle();   //唤醒阻塞在epoll_wait的线程
}

//扩容策略，设置成protected，以后修改策略比较方便
void IOManager::IOEventVecResize(size_t size)
{
    ioevents_vec.resize(size);
    for(size_t i=0;i<ioevents_vec.size();++i)
    {
        if(!ioevents_vec[i])
        {
            ioevents_vec[i]=new IOEvent;
            ioevents_vec[i]->fd=i;
        }
    }
}

//重载函数：判断是否可以停止，传入最近一个定时任务
//距离当前时刻的毫秒数
bool IOManager::stopping(uint64_t& timeout)
{
    timeout=GetNextTaskInterval();
    // std::cout<<"timeout="<<timeout<<"  pending_event_count="<<pending_event_count<<"  Scheduler::Stopping()="<<Scheduler::Stopping()
    //     <<std::endl;
    //没有定时任务  && 没有等待处理的事件 && 线程池中没有任务
    return timeout==std::chrono::milliseconds::max().count() && 
        pending_event_count==0 && Scheduler::Stopping();
}

//当线程空闲时，应当执行此方法
void IOManager::Idle()
{
    DEBUG(logger,"IDLE");
    //创建epoll_event数组
    const uint64_t MAX_EVENTS=256;
    epoll_event* events=new epoll_event[MAX_EVENTS];
    //智能指针不支持数组元素类型，需要提供一个删除函数
    std::shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr)
        {delete []ptr;});

    
    while(true)
    {
        uint64_t next_timeout=0;
        //std::cout<<"协程ID="<<HSQ::Fiber::GetFiberId()<<" 检测stopping(next_timeout)="<<stopping(next_timeout)<<std::endl;
        if(stopping(next_timeout))
        {
            DEBUG(logger,"协程退出,协程ID="+std::to_string(HSQ::Fiber::GetFiberId()));
            break;
        }
        int rt=0;
        do
        {
            //默认超时时间：ms
            static const uint64_t TIMEOUT=3000;
            if(next_timeout!=std::chrono::milliseconds::max().count())  {//有定时任务
                next_timeout=std::min(next_timeout,TIMEOUT);
               // std::cout<<"IOManager.cc:353 IOManager::Idle() 有定时任务：timeout="<< next_timeout<<std::endl;
            }
            else {//使用默认超时时间
                next_timeout=TIMEOUT;
            }
            rt=epoll_wait(ep_fd,events,MAX_EVENTS,next_timeout);
            if(rt<0 && errno==EINTR) {
                //超时返回，继续循环
            }
            else if (rt<0 && errno !=EINTR)  {
                auto msg=std::string("rt=" + std::to_string(rt)+"err: "+strerror(errno));
                ERROR(logger,msg);
            }
            else  {  //rt==0，没有任务，退出，
                break;
            }
        } while (true);
        std::vector<std::function<void()>> expired_tasks;
        GetExpiredTasks(expired_tasks);
        if(!expired_tasks.empty())
        {//对于超时任务，再次调度
            ASSERT(*expired_tasks.begin()!=nullptr,"*expired_tasks.begin()==nullptr  error");
            Schedule(expired_tasks.begin(),expired_tasks.end());
            expired_tasks.clear();
        }
        //处理epoll_wait返回的事件
        for(int i=0;i<rt;i++)
        {
            epoll_event& event=events[i];
            //如果是通过Tickle传递给管道的数据，读取后继续处理剩下的事件
            //因为是ET模式，需要一次读取完毕
            if(event.data.fd==pipe_fds[0])
            {
                uint8_t data[256];
                while(read(pipe_fds[0],data,sizeof(data))>0);
                continue;
            }
            //其他fd上的事件
            IOEvent* ioevent=(IOEvent*)events[i].data.ptr;
            std::lock_guard<std::mutex> lock(ioevent->ioevent_mt);

            //如果fd上出错或者连接断开，检查fd注册的类型，如果存在读写，添加上
            if(event.events &(EPOLLERR|EPOLLHUP))
                event.events |=(EPOLLIN | EPOLLOUT) & ioevent->ioevent_types;
            int real_event=IOEventType::NONE;
            //检测读写事件
            if(event.events & EPOLLIN)
                real_event|=IOEventType::READ;
            if(event.events & EPOLLOUT)
                real_event|=IOEventType::WRITE;
            
            //如果没有读写事件，继续处理其他fd
            if((ioevent->ioevent_types & real_event)==IOEventType::NONE)
                continue;
            
            //除了读写事件外，该fd上的其他事件
            int left_events=ioevent->ioevent_types& (~real_event);
            
            int op=left_events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;
            event.events=EPOLLET | left_events;

            int rt2=epoll_ctl(ep_fd,op,ioevent->fd,&event);
            if(rt2)
            {
                ERROR(logger,"ERROR(IOManager::Idle()) error");
                continue;
            }
            if(real_event & IOEventType::READ)
            {
                ioevent->TriggerIOEvent(IOEventType::READ);
                --pending_event_count;
            }
            if(real_event & IOEventType::WRITE)
            {
                ioevent->TriggerIOEvent(IOEventType::WRITE);
                --pending_event_count;
            }
        }
        Fiber::ptr cur=Fiber::GetCurrentRunningFiber();
        auto raw_ptr=cur.get();
        cur.reset();
       // DEBUG(logger, std::to_string(Scheduler::GetMainFiber()->GetFiberId()));
        raw_ptr->SwapOut();
    }
}

IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());

}



//***********************************************************
//namespace end 
}
