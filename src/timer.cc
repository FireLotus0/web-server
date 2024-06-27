#include"timer.h"
#include "macro.h"
static auto logger=GET_LOGGER("timer");


namespace HSQ
{

TimedTask::~TimedTask()
{

}

//只能在堆上创建
TimedTask::TimedTask(std::function<void()>&& task,uint64_t ms,Timer* ti,bool recycling)
{
    timed_task=std::move(task);
    interval_ms=Interval_MS(ms);
    is_recycling=recycling;
    start=std::chrono::steady_clock::now();
    trigger_point=start+Interval_MS(ms);
    timer=ti;
}
TimedTask::TimedTask(const std::function<void()>& task,uint64_t ms,Timer* ti,bool recycling)
{
    timed_task=task;
    interval_ms=Interval_MS(ms);
    is_recycling=recycling;
    start=std::chrono::steady_clock::now();
    trigger_point=start+Interval_MS(ms);
    timer=ti;
}
TimedTask::TimedTask(Time_Point point)
{
    trigger_point=point;
}

//************************************************************
bool  TimedTask::CancelTimedTask()
{
    std::lock_guard<std::mutex> lock(timer->mt);
    if(timed_task)
    {
        timed_task=nullptr;
        auto it=timer->tasks.find(shared_from_this());
        if(it!=timer->tasks.end())
            timer->tasks.erase(it);
        return true;
    }
    return false;
}

bool TimedTask::ResetTimedTask(uint64_t new_interval,bool from_now)
{
    auto new_ms=Interval_MS(new_interval);
    if(new_ms==interval_ms && !from_now)
        return true; 

    std::lock_guard<std::mutex> lock(timer->mt);
    if(timed_task==nullptr)
        return false;
    auto  it=timer->tasks.find(shared_from_this());
    if(it==timer->tasks.end())
        return false;
    //删除找到的定时任务
    timer->tasks.erase(it);
    //重新计算时间
    if(from_now)
        start=std::chrono::steady_clock::now();
    interval_ms=new_ms;
    trigger_point=start+interval_ms;
    timer->AddTimedTask(shared_from_this(),lock);
    return true;
    
}

bool TimedTask::Comparator::operator()(const TimedTask::ptr& lhs,const TimedTask::ptr &rhs)
{
    //lhs < rhs : true
    if(!lhs&&!rhs)
        return false;
    else if(!lhs)
        return true;
    else if(!rhs)
        return false;
    else if(lhs->trigger_point<rhs->trigger_point)
        return true;
    else if(lhs->trigger_point>rhs->trigger_point)
        return false;
    else
        return lhs.get()<rhs.get();
}

//********************************
//定时器
Timer::Timer()
{

}
Timer::~Timer()
{

}
//添加定时任务
TimedTask::ptr Timer::AddTimedTask(std::function<void()>&& task,uint64_t ms,bool recycling)
{
    //TimedTask构造参数使用右值引用，这里需要完美转发
    TimedTask::ptr task_ptr(new TimedTask(std::forward<std::function<void()>>(task),ms,this,recycling));
    std::lock_guard<std::mutex> lock(mt);
    AddTimedTask(task_ptr,lock);
    return task_ptr;
}
TimedTask::ptr Timer::AddTimedTask(const std::function<void()>& task,uint64_t ms,bool recycling)
{
    //TimedTask构造参数使用右值引用，这里需要完美转发
    TimedTask::ptr task_ptr(new TimedTask(task,ms,this,recycling));
    std::lock_guard<std::mutex> lock(mt);
    AddTimedTask(task_ptr,lock);
    return task_ptr;
}
//静态全局函数：响应条件定时任务
static void OnConditionTimedTask(std::weak_ptr<void>cond,std::function<void()> cb)
{
    auto temp=cond.lock();
    if(temp)
        cb();
}
//添加定时任务附带条件，触发时会检查条件,绑定到静态全局函数
TimedTask::ptr Timer::AddTimedTask(std::function<void()>&& task,uint64_t ms,std::weak_ptr<void> condition,bool recycling)
{
    return AddTimedTask(std::bind(&OnConditionTimedTask,condition,task),ms,recycling);
}
//获取小顶堆 堆顶定时任务距离现在还有多少ms触发
uint64_t Timer::GetNextTaskInterval()
{
    std::lock_guard<std::mutex> lock(mt);
    //没有定时任务
    if(tasks.empty())
        return std::chrono::milliseconds::max().count();
    trigger=false;
    const auto& task_ptr=*tasks.begin();
    auto now_time_point=std::chrono::steady_clock::now();
    if(now_time_point>=task_ptr->trigger_point)
    {
        //std::cout<<now_time_point.time_since_epoch().count()<<"------"<<task_ptr->trigger_point.time_since_epoch().count()<<std::endl;
        return 0;
    }  
    else
        return std::chrono::duration_cast<std::chrono::milliseconds>(task_ptr->trigger_point-now_time_point).count();
}
//获取已经超时的定时任务
void Timer::GetExpiredTasks(std::vector<std::function<void()>>&expired_tasks)
{
    std::lock_guard<std::mutex> lock(mt);
    if(tasks.empty())
        return;
    std::list<TimedTask::ptr> expired;
    //set容器根据TimedTask的trigger_point进行排序，所以创建一个临时TimedTask与set中的元素进行比较
    TimedTask::ptr now_time_point(new TimedTask(std::chrono::steady_clock::now()));
    //set容器lower_bound返回第一个大于等于给定元素的迭代器
    auto it=tasks.lower_bound(now_time_point);
    
    while(it!=tasks.end() && (*it)->trigger_point==now_time_point->trigger_point)
        ++it;
    //将超时的任务记录
    expired.insert(expired.begin(),tasks.begin(),it);
    //移除超时任务
    tasks.erase(tasks.begin(),it);
    for(auto & t: expired)
    {
        if(t->is_recycling)
        {
            t->trigger_point=std::chrono::steady_clock::now()+t->interval_ms;
            tasks.insert(t);
        }
        expired_tasks.push_back(t->timed_task);
    }
}
//是否具有定时任务
bool Timer::HasTimedTask()
{
    std::lock_guard<std::mutex> lock(mt);
    return tasks.size()>0;
}
//添加定时器任务--辅助函数
void Timer::AddTimedTask(TimedTask::ptr task,std::lock_guard<std::mutex> &lock)
{
    auto it=tasks.insert(task).first;
    bool at_front=(it==tasks.begin()) && !trigger;
    if(at_front)
        trigger=true;
    lock.~lock_guard();

    if(at_front)
        InsertAtFront();    //纯虚函数，由子类做出处理
}


//************************************************************
//namespace  end
}