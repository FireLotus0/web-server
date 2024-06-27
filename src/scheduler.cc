#include"scheduler.h"

namespace  HSQ
{
//调试日志
static auto g_logger=GET_LOGGER("system");

static thread_local Scheduler* t_scheduler=nullptr;
//t_scheduler_fiber:
//对于线程池中的线程，其指向主协程t_local_main_fiber
//对于创建线程池的线程，其指向运行run函数的协程
static thread_local Fiber* t_local_main_fiber=nullptr;

//Task成员函数实现
Scheduler::Task::Task()
{
    certain_id=ThreadId();
    func_task=nullptr;
    fiber_task=nullptr;
}
Scheduler::Task::Task(std::function<void()>&&cb,ThreadId id)
{
    func_task=std::move(cb);
    certain_id=id;
}
Scheduler::Task::Task(const std::function<void()>& cb,ThreadId id)
{
    func_task=cb;
    certain_id=id;
}
Scheduler::Task::Task(HSQ::Fiber::ptr &&fiber,ThreadId id)
{
    fiber_task=fiber;
    certain_id=id;
}
Scheduler::Task::Task(const HSQ::Fiber::ptr& fiber,ThreadId id)
{
    fiber_task=fiber;
    certain_id=id;
}
//清空任务
void Scheduler::Task::Reset()
{
    certain_id=ThreadId();
    func_task=nullptr;
    fiber_task=nullptr;
}



//构造函数：参数--线程数量，bool use_caller,调度器名称（方便日志）
Scheduler::Scheduler(uint32_t threads,bool use_caller,const std::string name)
{
    scheduler_name=name;
    ASSERT(threads>0,"Scheduler::Scheduler()  error: threads <=0");

    if(use_caller)
    {
        //如果当前线程中没有协程，将会创建主协程
        HSQ::Fiber::GetCurrentRunningFiber();
        --threads;
        ASSERT(GetScheduler()==nullptr,"Scheduler::Scheduler() GetScheduler()!=nullptr");
        
        //将线程局部变量t_scheduler初始化
        t_scheduler=this;
        //创建scheduler协程，绑定run函数
        schedule_fiber.reset(new Fiber(std::bind(&Scheduler::Run,this),128*1024,true));
        //初始化线程局部变量t_local_main_fiber
        t_local_main_fiber=schedule_fiber.get();// HSQ::Fiber::GetCurrentRunningFiber().get();//
        //保存主线程ID
        main_thread=HSQ::GetThreadId();
        thread_ids.emplace_back(main_thread);
    }
    else {
        main_thread=0;
    }
        
    thread_count=threads;
    //DEBUG(g_logger,"Scheduler 构造成功");
}
    
Scheduler::~Scheduler()
{
    //调度器析构时，必然已经停止
    ASSERT(is_stopping,"Scheduler::~Scheduler() error: is_stopping=false");
    if(GetScheduler()==this)
        t_scheduler=nullptr;
}


std::string Scheduler::GetSchedulerName()const
{
    return scheduler_name;
}


 //启动调度器：创建线程池
void Scheduler::Start()
{
    std::lock_guard<std::mutex> lock(mt);
    //检查当前是否已经在运行中
    if(!is_stopping)
        return;
    is_stopping=false;
    ASSERT(thread_pool.empty(),"Scheduler::start() thread_pool.empty()==false");
    for(size_t i=0;i<thread_count;++i)
    {
        thread_pool.emplace_back(std::move(std::thread(&Scheduler::Run,this)));
        thread_ids.push_back(HSQ::GetThreadId());
    }
}

//停止调度器
void Scheduler::Stop()
{
    //DEBUG(g_logger,"调用Stop");
    // {
    //     std::lock_guard<std::mutex> lock(mt);
    //     dump(std::cout);
    // }
    
    //设置将要停止标记，但是当前不一定能停止，需要等待其他线程处理完任务
    is_autostop=true;
    //创建了schedule_fiber的前提下：
    if(schedule_fiber && thread_count==0 &&
        (schedule_fiber->GetFiberState()==Fiber::State::TERM || 
        schedule_fiber->GetFiberState()==Fiber::State::INIT))
    {
        DEBUG(g_logger,"Scheduler is stopping! ");
        is_stopping=true;

        //在stopping函数中，检测是否可以停止
        if(Stopping())
            return;
    }
   // DEBUG(g_logger,"can not quit! ");
    //执行到这里，说明退出条件不满足，还有剩余的任务要处理
    //或者use_caller==false，没有创建schedule_fiber
    if(main_thread!=0)
    {
        //说明是主线程,创建了schedule_fiber，并且原因是退出条件不满足
        ASSERT(GetScheduler()==this,"main_thread!=0 but GetScheduler()!=this");
    }
    else
    {
        //说明创建调度器的线程是线程池中的线程，use_caller==false，并且原因是没有
        //创建schedule_fiber
        ASSERT(GetScheduler()!=this,"main_thread!=0 but GetScheduler()==this");
    }

    is_stopping=true;

    //每次调用tickle()都会将阻塞在epoll_wait的线程唤醒
    //如果当前任务很少，线程池里面的线程全部阻塞在epoll_wait，
    //则tickle thread_count次通知每个线程停止即可。
    
    for(size_t i=0;i<thread_count;++i)
        Tickle();
    //如果任务仍然很多，阻塞在epoll_wait的线程很少,tickle  thread_count次
    //仍然不能通知到每个线程。当前线程的调度协程将会参与任务处理
    if(schedule_fiber)
    {
      //  DEBUG(g_logger,"!Stopping()" + !Stopping() ? "调用call" : "输出156");
        if(!Stopping())
            schedule_fiber->Call();
    }
    //如果当前线程也参与了任务处理，则当前线程将会一直处理任务，直到任务队列为空
    //调用epoll_wait,查看停止标记，然后退出
    //此时任务已经处理完毕
    //std::lock_guard<std::mutex> lock(mt);
    for(auto& t:thread_pool) {
        DEBUG(g_logger,"thread join");
        t.join();
    }
        
}

//静态函数
//返回当前协程调度器的指 Scheduler()
Scheduler* Scheduler::GetScheduler()
{
    return t_scheduler;
}

//返回当前线程中的主协程
Fiber* Scheduler::GetMainFiber()
{
    return t_local_main_fiber;
}


//向epoll_wait中写入数据，唤醒等待epoll_wait的线程
void Scheduler::Tickle()
{
    DEBUG(g_logger,"Tickle");
}

//当线程空闲时，应当执行此方法
void Scheduler::Idle()
{
    DEBUG(g_logger,"Idle");
    while(!Stopping())
    {
        HSQ::Fiber::YieldToHold();//将当前的运行的协程状态修改为HOLD，退出cpu
    }
        
}



//设置当前调度器
void Scheduler::SetCurrentScheduler(Scheduler* sc)
{
    t_scheduler=sc;
}

//判断是否有空闲线程
bool Scheduler::HasIdleThread()
{
    return idle_thread_count.load(std::memory_order_acquire)>0;
}

//返回是否可以停止：当前任务处理完成才停止
bool Scheduler::Stopping()
{
    std::lock_guard<std::mutex> lock(mt);
    //std::cout<<"task_queue.size()="<<task_queue.size()<<std::endl;
    
    return is_autostop && is_stopping
        && active_thread_count.load(std::memory_order_acquire)==0
        && task_queue.empty();
}

std::ostream& Scheduler::dump(std::ostream& os)
{
    os << "[Scheduler name=" << scheduler_name
       << " thread_pool size=" << thread_count
       << " active_thread_count=" << active_thread_count.load(std::memory_order_acquire)
       << " idle_count=" << idle_thread_count.load(std::memory_order_acquire)
       << " stopping=" << is_stopping
       << " ]" << std::endl << "    ";
    for(auto&& t:thread_ids)
    {
        if(t!=ThreadId())
            os<<t<<",  ";
    }
    return os;
}

//调度函数：线程池中的线程以此作为线程任务函数
void Scheduler::Run()
{
 //   DEBUG(g_logger,"run");
    HSQ::SetHook(true);
    SetCurrentScheduler(this);
    //线程池中的工作线程
    if(HSQ::GetThreadId()!=main_thread)
    {
         t_local_main_fiber=HSQ::Fiber::GetCurrentRunningFiber().get();
    }

    //创建Idle协程，当没有任务时切换到idle协程运行
    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::Idle,this)));

    Fiber::ptr cb_fiber;

    Task task;

    while(true)
    {
        task.Reset();
        bool need_tickle=false;
        bool is_active=false;
       // DEBUG(g_logger,"in while loop");
        {
            std::lock_guard<std::mutex> lock(mt);
          //  DEBUG(g_logger,"lock");
            auto it=task_queue.begin();
            while(it!=task_queue.end())
            {

                if(it->certain_id!=0 && it->certain_id!=HSQ::GetThreadId())
                {
                    //当前任务指定了线程，但不是本线程，不做处理
                    //need_tickle=true，将会唤醒epoll_wait,让那个线程
                    //有机会取走自己的任务
                    need_tickle=true;
                    ++it;
                    continue;
                }
                ASSERT(it->fiber_task || it->func_task,"任务队列中的对象没有有效任务");
                if(it->fiber_task && it->fiber_task->GetFiberState()==HSQ::Fiber::State::EXEC)
                {
                    //取得任务为协程，但任务在运行中
                    ++it;
                    continue;
                }
                task=std::move(*it);
                task_queue.erase(it);
                ++active_thread_count;
                is_active=true;
                break;
            //    DEBUG(g_logger,"get task");

            }
            need_tickle |=(!task_queue.empty());
         //   DEBUG(g_logger,"lock mt shoule release");

        }
        if(need_tickle)
            Tickle();
      //  DEBUG(g_logger,"after tickle");
        if(task.fiber_task && task.fiber_task->GetFiberState()!=Fiber::State::TERM
            && task.fiber_task->GetFiberState()!=Fiber::State::EXCEPT)
        {
            //任务运行
         //   DEBUG(g_logger,"before SwapIn");
            task.fiber_task->SwapIn();
          //  DEBUG(g_logger,"after SwapIn");

            --active_thread_count;

            //判断运行结果
            if(task.fiber_task->GetFiberState()==Fiber::State::READY)
                Schedule(std::move(task.fiber_task));
            else if(task.fiber_task->GetFiberState()!=Fiber::State::TERM
                && task.fiber_task->GetFiberState() !=Fiber::State::EXCEPT)
                task.fiber_task->fiber_state=Fiber::State::HOLD;

            task.Reset();
        }
        else if(task.func_task)
        {
           // DEBUG(g_logger,"task.func_task");
            if(cb_fiber)
                cb_fiber->ResetFiberTask(std::move(task.func_task));
            else
                cb_fiber.reset(new Fiber(std::move(task.func_task)));
            task.Reset();
            cb_fiber->SwapIn();
           // INFO(g_logger, "cb_fiber->SwapIn");
            --active_thread_count;
            //判断运行结果
            //INFO(g_logger, "rask run result: " + FiberStateToString(cb_fiber->GetFiberState()));

            if(cb_fiber->GetFiberState()==Fiber::State::READY) {
              //  DEBUG(g_logger,"schedule in 334");
                Schedule(std::move(cb_fiber));
              //  DEBUG(g_logger,"schedule in 336");

            }
            else if(cb_fiber->GetFiberState()==Fiber::State::TERM
                || cb_fiber->GetFiberState() ==Fiber::State::EXCEPT) {
                cb_fiber->ResetFiberTask(nullptr);
            }
            else {
                cb_fiber->fiber_state=Fiber::State::HOLD;
                cb_fiber.reset();
            }
            
        }
        else
        {
           // DEBUG(g_logger,"last else");
            if(is_active)
            {
                --active_thread_count;
                continue;
            }
            if(idle_fiber->GetFiberState()==Fiber::State::TERM)
            {
                DEBUG(g_logger,"idle fiber term");
                break;
            }

            ++idle_thread_count;
            idle_fiber->SwapIn();
            --idle_thread_count;
            if(idle_fiber->GetFiberState()!=Fiber::State::EXCEPT
                && idle_fiber->GetFiberState() !=Fiber::State::TERM)
                idle_fiber->fiber_state=Fiber::State::HOLD;
        }
    }

}

Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

}