#pragma once

#include<thread>
#include<list>
#include<memory>
#include<functional>
#include<atomic>
#include<string>
#include<mutex>

#include"fiber.h"
#include"util.h"
#include"macro.h"
#include"hook.h"
namespace HSQ
{


//***********************************************************
//协程调度器：包含了线程池的功能，改写了同步队列的使用。此处后续需要优化。如果不对最大线程数量做限制，当并发量很大时，会创建很多
//线程，可能导致内存暴涨
class Scheduler
{
public:
    using ptr=std::shared_ptr<Scheduler>;
    using ThreadId=unsigned long long;
    //封装调度对象：任务队列中可以存储函数，协程
    struct Task
    {
        std::function<void()> func_task;
        HSQ::Fiber::ptr fiber_task;
        ThreadId certain_id; //标识任务是否指定在某个线程上运行
        Task();

        Task(std::function<void()>&&cb,ThreadId id=ThreadId());
        
        Task(const std::function<void()>& cb,ThreadId id=ThreadId());
       
        Task(HSQ::Fiber::ptr &&fiber,ThreadId id=ThreadId());
       
        Task(const HSQ::Fiber::ptr& fiber,ThreadId id=ThreadId());
       
        //清空任务
        void Reset();
      
    };
public:

    //构造函数：参数--线程数量，bool use_caller,调度器名称（方便日志）
    Scheduler(uint32_t threads=1,bool use_caller=true,const std::string name="Scheduler");
    
    ~Scheduler();

    Scheduler(const Scheduler&)=delete;
    Scheduler& operator=(const Scheduler&)=delete;
    Scheduler(Scheduler&&)=delete;
    Scheduler& operator=(Scheduler&&)=delete;
    //获取调度器名称
    std::string GetSchedulerName()const;

    //启动调度器：创建线程池
    void Start();
  
    //返回线程数，方便静态函数调用
    uint64_t GetThreadCounts();

    std::string getName() const { return scheduler_name; }

    //停止调度器
    void Stop();

    //调度任务：将任务加入到任务队列中
    //调度单个任务
    template<typename T>
    void Schedule(T && task,ThreadId id=ThreadId())
    {
        bool need_tickle=false;
        {
            //这里用自旋锁会不会更好呢？锁住的时间比较短，用lock_guard相对开销大
            std::lock_guard<std::mutex> lokc(mt);
            need_tickle = ScheduleLockFree(std::forward<T>(task),id);
        }
        if(need_tickle)
            Tickle();
    }
    //批量调度任务
    template<typename Iter>
    void Schedule(Iter begin,Iter end)
    {
        bool need_tickle=false;
        {
            std::lock_guard<std::mutex> lock(mt);
            while(begin!=end)
            {
                need_tickle |=ScheduleLockFree(std::move(*begin),ThreadId());
                ++begin;
            }
        }
        if(need_tickle)
            Tickle();
    }

    //输出当前状态：
    std::ostream& dump(std::ostream& os);
public://静态函数
    //返回当前协程调度器的指针
    static Scheduler* GetScheduler();

    //返回当前线程中的主协程
    static HSQ::Fiber* GetMainFiber();

 static Scheduler* GetThis();
 
protected: 
    //向epoll_wait中写入数据，唤醒等待epoll_wait的线程
    virtual void Tickle();

    //当线程空闲时，应当执行此方法
    virtual void Idle();

    //调度函数：线程池中的线程以此作为线程任务函数
    void Run();

    //设置当前调度器
    void SetCurrentScheduler(Scheduler* sc);

    //判断是否有空闲线程
    bool HasIdleThread();

    //返回是否可以停止：当前任务处理完成才停止
    virtual bool Stopping();

private:
    //读写互斥量：保护任务队列
    std::mutex mt;
    //线程池
    std::list<std::thread> thread_pool;
    //任务队列
    std::list<Task> task_queue;
    //运行run函数的协程
    Fiber::ptr schedule_fiber;
    //调度器名称
    std::string scheduler_name;

    //辅助函数：在schedule调度单个任务，调度一组任务都会使用这段代码
    template<typename T>
    bool ScheduleLockFree(T && task,ThreadId id=ThreadId())
    {
        DEBUG(GET_LOGGER("system"),"SChedule : cur fiber is " + std::to_string(Fiber::GetFiberId()));
        //如果任务队列为空，可能各个线程都在处理任务
        //此时，应当唤醒epoll_wait，取出其中的任务，添加到任务队列中
        bool need_tickle=task_queue.empty();

        Task t(std::forward<T>(task),id);
        if(t.fiber_task||t.func_task) {
            task_queue.emplace_back(std::move(t));
        }
       // std::cout << "need_tickle=" << need_tickle << " in scheduler.cc line:155" << std::endl;
        return need_tickle;
    }
    
protected:
    //线程id数组
    std::list<ThreadId> thread_ids;
    //当前线程ID，区分线程池中的线程和当前线程
    ThreadId main_thread=0;
    //正在停止标记
    std::atomic<bool> is_stopping{true};
    //自动停止标记：当调度器调用stop()，不能立即停止，需要等待其他线程处理完自己的任务
    std::atomic<bool> is_autostop{false};
    //线程数量
    size_t thread_count{0};
    //空闲线程数量
    std::atomic<size_t> idle_thread_count{0};
    //活跃线程数量
    std::atomic<size_t> active_thread_count{0};

};

//***********************************************************
//namespace end   
}
