#pragma once

#include<cstdint>
#include<functional>
#include<ucontext.h>
#include<atomic>
#include<memory>


/**********************************************************
 * 协程对象的封装：class  Fiber
 * 基于ucontext
 * 程序中的协程模块（包括协程调度，io管理）本质是一个线程池
 * 传统的线程池设计模式包括半同步-半异步模式，领导者-追随者模式
 * 其中半同步半异步模式包括：同步服务层---同步队列层---异步服务层
 * 调度器（class  Scheduler）属于同步队列层和异步服务层
 * 传统做法是一个主线程负责监听事件，当事件发生时，将事件丢到同步队列里面
 * 异步服务层（工作线程）从同步队列里面取出事件，并行执行。这种模式对于
 * 协程来说并不适用，因为，同步队列为空或者满了，线程会阻塞，导致协程不能调度，
 * 所以，当同步队列没有任务时，协程应当继续执行（程序中将陷入epoll_wait）。
 * 创建调度器的线程为主线程，线程池中的线程为工作线程，这两种线程的执行逻辑是不一样的，需要进行区分。
 * 在工作线程中，线程要么在处理协程任务，要么就陷入epoll_wait等待事件发生。
 * 在主线程中，包括启动调度器，关闭调度器，管理线程池，主要任务不是运行协程任务。
 * **********************************************************/

namespace HSQ
{
//***********************************************************
//用于为协程分配栈空间

class Fiber:public std::enable_shared_from_this<Fiber>
{
//Schduler可以访问Fiber的私有成员
friend class Scheduler;
public:
    enum State
    {
        INIT=0,   //初始状态，未运行过
        EXEC=1,   //运行中状态
        HOLD=2,   //暂停状态，如所需外部条件未达成，进入此状态
        READY=3,  //可以运行状态，如让出cpu给其他协程进入此状态
        TERM=4,   //正常终止状态
        EXCEPT=5  //执行异常状态
    };
public:
    using ptr=std::shared_ptr<Fiber>;
    Fiber();    //默认构造函数，创建主协程
    Fiber(std::function<void()>&& cb,uint32_t size=128*1024,bool use_caller=false);
    ~Fiber();


    //重置任务：INIT，TERM，EXCEPT状态的协程可以重置
    void ResetFiberTask(std::function<void()>&&cb);

    //协程切换：工作线程中的协程调用SwapIn,SwapOut。主线程中的协程调用call ,back
    void SwapIn();  
    void SwapOut();

    void Call();
    void Back();

    State GetFiberState()const;
    uint32_t  GetId()const;  //给GetFiberId使用    

public:
    //输出到日志
    static uint32_t GetFiberId();

    static void YieldToReady();

    static void YieldToHold();

    //上下文绑定的执行函数：工作线程中的协程绑定WorkerThreadFiberFunc,主线程中的协程绑定MainThreadFiberFunc
    static void WorkerThreadFiberFunc();
    static void MainThreadFiberFunc();

    //统计协程数
    static uint64_t GetTotalFibers();

    //将线程局部变量t_cur_running_fiber指针指向自己
    static void SetCurrentRunningFiber(Fiber* ptr);

    //获取当前线程中用于调度时保存寄存器状态的协程
    static Fiber::ptr GetCurrentRunningFiber();

private:
    uint32_t fiber_id=0;                  //协程id
    std::function<void()> fiber_task=nullptr;   //任务函数
    ucontext_t fiber_context;           //ucontext上下文，保存寄存器状态信息
    State fiber_state=INIT;                  //协程状态
    void* fiber_stack=nullptr;                  //使用有栈协程
    uint32_t fiber_satck_size=128*1024;          //栈空间大小
};

//协程状态转换函数：将状态转换成string，便于日志输出
std::string FiberStateToString(Fiber::State state);

//***********************************************************
//namespace end
}