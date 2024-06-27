#include"fiber.h"
#include"macro.h"
#include"scheduler.h"
#include"util.h"


namespace HSQ
{
//***********************************************************

// //调试日志
static HSQ::Logger::ptr g_logger=GET_LOGGER("system");


static std::atomic<uint32_t> s_fiber_id{0};
static std::atomic<uint64_t> s_fiber_count{0};

//static与thread_local连用，修饰文件可见性
static thread_local Fiber* t_local_running_fiber=nullptr;
static thread_local Fiber::ptr t_local_main_fiber=nullptr;

//内存分配:将来若要改变分配策略，修改此处即可
class Alloctor
{
public:
    static void* Alloc(size_t size)
    {
        return malloc(size);
    }
    static void Dealloc(void* ptr,size_t size)
    {
        free(ptr);
    }
};

//默认构造函数，创建主协程
Fiber::Fiber()
{
    fiber_state=EXEC;
    //将当前线程中正在运行的协程设为自己
    SetCurrentRunningFiber(this);

    //getcontext(&fiber_context)成功返回0
    ASSERT((!getcontext(&fiber_context)),"Fiber::Fiber()  getcontext error");
    //创建成功，增加计数
    fiber_id=++s_fiber_id;
    DEBUG(g_logger,"协程创建 协程ID="+std::to_string(fiber_id));
    ++s_fiber_count;
}   
Fiber::Fiber(std::function<void()>&& cb,uint32_t size,bool use_caller)
{
    fiber_id=++s_fiber_id;
    fiber_task=std::move(cb);
    ++s_fiber_count;
    //分配空间
    fiber_stack=Alloctor::Alloc(size);
    //保存当前上下文
    ASSERT(!getcontext(&fiber_context),"Fiber::Fiber(std::function<void()>&& ,uint32_t,bool use_caller) error");
    //设置栈空间指针
    fiber_context.uc_link=nullptr;
    fiber_context.uc_stack.ss_sp=fiber_stack;
    fiber_context.uc_stack.ss_size=fiber_satck_size;
    //绑定执行函数，当当前上上下文加载进cpu后将要执行的函数入口地址
    if(use_caller) {
        makecontext(&fiber_context,&Fiber::MainThreadFiberFunc,0);
    } else  {
        makecontext(&fiber_context,&Fiber::WorkerThreadFiberFunc,0);
    }
    //DEBUG(g_logger,"Fiber::Fiber(std::function<void()>&& ,uint32_t,bool use_caller) sucess fiber_id="+std::to_string(fiber_id));
    DEBUG(g_logger,"协程创建 协程ID="+std::to_string(fiber_id));
}
Fiber::~Fiber()
{
    //判断是否是主协程，主协程没有栈空间，没有绑定函数，由默认构造函数创建
    if(fiber_stack)
    {//工作协程
        ASSERT(fiber_state==TERM||fiber_state==EXCEPT||fiber_state==INIT,"Fiber::~Fiber() error"+FiberStateToString(fiber_state));
        //回收栈空间
        Alloctor::Dealloc(fiber_stack,fiber_satck_size);
    }
    else
    {//主协程析构时，工作协程必须先全部析构，并且此时正在运行的协程必然为主协程
        ASSERT(fiber_task==nullptr,"协程析构错误: 主协程中fiber_task!=nullptr");
        ASSERT(fiber_state==Fiber::State::EXEC,"协程析构错误: 主协程中fiber_state!=EXEC fiber_state="+FiberStateToString(fiber_state));
        ASSERT(t_local_running_fiber==this,"协程析构错误: t_local_running_fiber!=this");
        if(t_local_running_fiber==this)
            SetCurrentRunningFiber(nullptr);
    }
    --s_fiber_count;
    DEBUG(g_logger,"协程析构 协程ID="+std::to_string(fiber_id));
}


//重置任务：INIT，TERM，EXCEPT状态的协程可以重置
void Fiber::ResetFiberTask(std::function<void()>&&cb)
{//工作协程才能重置

    ASSERT(fiber_state!=Fiber::State::EXEC&&fiber_state!=Fiber::State::HOLD&&fiber_state!=Fiber::State::READY
            ,"ResetFiberTask() error : fiber_state="+FiberStateToString(fiber_state));
    ASSERT(fiber_stack!=nullptr,"ResetFiberTask() error : fiber_stack==nullptr");
    //重置状态
    fiber_state=Fiber::State::INIT;
    fiber_task=std::move(cb);
    ASSERT((!getcontext(&fiber_context)),"Fiber::RestFiberTask getcontext error");
    fiber_context.uc_link=nullptr;
    fiber_context.uc_stack.ss_sp=fiber_stack;
    fiber_context.uc_stack.ss_size=fiber_satck_size;
    
    makecontext(&fiber_context,&Fiber::WorkerThreadFiberFunc,0);

}

//协程切换：工作线程中的协程调用SwapIn,SwapOut。主线程中的协程调用call ,back
void Fiber::SwapIn()
{
   // DEBUG(g_logger, "call SwapIn and task run");
    SetCurrentRunningFiber(this);
    ASSERT(fiber_state!=Fiber::State::EXEC,"Fiber::SwapIn() error: fiber_state==Fiber::State::EXEC");
    fiber_state=Fiber::State::EXEC;
    ASSERT((!swapcontext(&Scheduler::GetMainFiber()->fiber_context,&fiber_context)),"Fiber::SwapIn() error: \
          swapcontext(&Scheduler::GetMainFiber()->fiber_context,&fiber_context)");
   // DEBUG(g_logger, " task run over");
} 
void Fiber::SwapOut()
{
    ASSERT(Scheduler::GetMainFiber()!=this,"Scheduler::GetMainFiber()  error: Scheduler::GetMainFiber()==this");
    SetCurrentRunningFiber(Scheduler::GetMainFiber());
    ASSERT((!swapcontext(&fiber_context,&Scheduler::GetMainFiber()->fiber_context)),"Fiber::SwapOut() error: \
            swapcontext(&fiber_context,&Scheduler::GetMainFiber()->fiber_context)");
}

//call()：由创建IO调度器，并且use_caller=true时创建的run协程调用,主要用于停止时，由该协程加载进cpu，清理剩余的任务
void Fiber::Call()
{
       std::string msg = std::to_string((unsigned long long)(t_local_main_fiber.get())) + "---" + 
                std::to_string((unsigned long long)(Scheduler::GetMainFiber())) + "---" +
                std::to_string((unsigned long long)(t_local_running_fiber));
    SetCurrentRunningFiber(this); 
    fiber_state=Fiber::State::EXEC;
 
   // INFO(g_logger, msg);
    
    ASSERT((!swapcontext(&t_local_main_fiber->fiber_context,&fiber_context)),"Fiber::Call() error");
}
//Back()：由创建IO调度器，并且use_caller=true时创建的run协程调用,
void Fiber::Back()
{
    ASSERT(Scheduler::GetMainFiber()!=this,"Scheduler::GetMainFiber()  error: Scheduler::GetMainFiber()==this");
    SetCurrentRunningFiber(t_local_main_fiber.get());
    ASSERT((!swapcontext(&fiber_context,&t_local_main_fiber->fiber_context)),"Fiber::Back() error");
}

Fiber::State Fiber::GetFiberState()const
{
    return fiber_state;
}
uint32_t  Fiber::GetId()const
{
    return fiber_id;
}   

uint32_t  Fiber::GetFiberId()
{
    if(t_local_running_fiber)
        return t_local_running_fiber->GetId();
    return 0;
}

void Fiber::YieldToReady()
{
    ASSERT(Fiber::GetCurrentRunningFiber()!=t_local_main_fiber,"Fiber::YieldToReady() error: Fiber::GetCurrentRunningFiber()!=t_local_main_fiber");
    Fiber::ptr cur=Fiber::GetCurrentRunningFiber();
    ASSERT(cur->fiber_state==Fiber::State::EXEC,"Fiber::YieldToReady() error: cur->fiber_state!=Fiber::State::EXEC");
    cur->fiber_state=Fiber::State::READY;
    cur->SwapOut();
}

void Fiber::YieldToHold()
{
    ASSERT(Fiber::GetCurrentRunningFiber()!=t_local_main_fiber,"Fiber::GetCurrentRunningFiber()==t_local_main_fiber");

    Fiber::ptr cur=Fiber::GetCurrentRunningFiber();
    ASSERT(cur!=nullptr,"Fiber::GetCurrentRunningFiber()==nullptr");
    ASSERT(cur->fiber_state==Fiber::State::EXEC,"Fiber::YieldToReady() error: cur->fiber_state!=Fiber::State::EXEC");
    cur->fiber_state=Fiber::State::HOLD;
    cur->SwapOut();
}

//上下文绑定的执行函数：工作线程中的协程绑定WorkerThreadFiberFunc,主线程中的协程绑定MainThreadFiberFunc
void Fiber::WorkerThreadFiberFunc()
{
    Fiber::ptr cur=GetCurrentRunningFiber();
    ASSERT(cur->fiber_task!=nullptr,"Fiber::WorkerThreadFiberFunc() error: fiber_task==nullptr");
    //执行协程绑定的函数
    try
    {
        cur->fiber_task();
        cur->fiber_state=Fiber::State::TERM;
        cur->fiber_task=nullptr;
    }
    catch(const std::exception& e)
    {
        cur->fiber_state=Fiber::State::EXCEPT;
        ERROR(g_logger,e.what());
    }
    catch(...)
    {
        cur->fiber_state=Fiber::State::EXCEPT;
        ERROR(g_logger,"catch(...)   error");
    }

    auto raw_ptr=cur.get();
    cur.reset();
    raw_ptr->SwapOut();

    ASSERT(false,"执行流程错误，不应当到达此处：Fiber::WorkerThreadFiberFunc()");
}
void Fiber::MainThreadFiberFunc()
{
    Fiber::ptr cur=GetCurrentRunningFiber();
    ASSERT(cur->fiber_task!=nullptr,"Fiber::WorkerThreadFiberFunc() error: fiber_task==nullptr");
    //执行协程绑定的函数
    try
    {
        cur->fiber_task();
        cur->fiber_state=Fiber::State::TERM;
        cur->fiber_task=nullptr;
    }
    catch(const std::exception& e)
    {
        cur->fiber_state=Fiber::State::EXCEPT;
        ERROR(g_logger,e.what());
    }
    catch(...)
    {
        cur->fiber_state=Fiber::State::EXCEPT;
        ERROR(g_logger,"catch(...)   error");
    }

    auto raw_ptr=cur.get();
    cur.reset();
    raw_ptr->Back();

    ASSERT(false,"执行流程错误，不应当到达此处：Fiber::MainThreadFiberFunc()");
}

//统计协程数
uint64_t Fiber::GetTotalFibers()
{
    return s_fiber_count;
}

//将线程局部变量t_cur_running_fiber指针指向自己
void Fiber::SetCurrentRunningFiber(Fiber* ptr)
{
    t_local_running_fiber=ptr;
}

//获取当前线程中用于调度时保存寄存器状态的协程
Fiber::ptr Fiber::GetCurrentRunningFiber()
{
    if(t_local_running_fiber)
        return t_local_running_fiber->shared_from_this();
    Fiber::ptr main_fiber(new Fiber()); //默认构造函数中已经调用SetCurrentRunningFiber
    ASSERT(t_local_running_fiber==main_fiber.get(),"GetCurrentRunningFiber() error :t_local_running_fiber!=main_fiber ");
    t_local_main_fiber=main_fiber; 
    return t_local_running_fiber->shared_from_this();
}

//辅助函数
std::string FiberStateToString(Fiber::State state)
{
    switch(state)
    {
        case Fiber::State::INIT: return "INIT";
        case Fiber::State::EXEC: return "EXEC";
        case Fiber::State::HOLD: return "HOLD";
        case Fiber::State::READY: return "READY";
        case Fiber::State::TERM: return "TERM";
        case Fiber::State::EXCEPT: return "EXCEPT";
        default: return "UNDEFINE ERROR";
    }
}

//***********************************************************
//namespace end
}