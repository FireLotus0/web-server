//定时器实现，定时器使用一个小顶堆，c++中可以使用标准库set,传入一个函数对象或者lambda表达式对元素进行比较
#pragma once

#include<memory>
#include<chrono>
#include<set>
#include<functional>
#include<mutex>
#include<list>
#include<vector>

#include<iostream>

namespace HSQ
{
//************************************************************
/**********************
 * 定时任务：
 *      要执行的任务----function<void()>
 *      是否是循环执行:bool 
 *      定时的时间间隔：uint64_t ms
 *      下次触发的时间点：
 *      取消和重置定时任务
 *  **/
//时长： ms
using Interval_MS=std::chrono::milliseconds;
//时间点
using Time_Point=std::chrono::time_point<std::chrono::steady_clock>;

class Timer;    //类前置声明

class TimedTask:public std::enable_shared_from_this<TimedTask>
{
friend class Timer;
public:
    using ptr=std::shared_ptr<TimedTask>;

   ~TimedTask();

    //取消任务
    bool  CancelTimedTask();

    bool ResetTimedTask(uint64_t new_interval,bool from_now);

private:
     TimedTask(std::function<void()>&& task,uint64_t ms,Timer* ti,bool recycling=false);
     TimedTask(const std::function<void()>& task,uint64_t ms,Timer* ti,bool recycling=false);
     TimedTask(Time_Point point);
     
private:
    std::function<void()> timed_task;    //任务
    Interval_MS interval_ms;            //时间周期
    Time_Point start;                   //计时起点
    bool is_recycling;                  //是否循环
    Time_Point trigger_point;           //触发时间点
    HSQ::Timer* timer;
private:
    //比较运算符，提供给set容器使用
    struct Comparator
    {
        bool operator()(const TimedTask::ptr& lhs,const TimedTask::ptr &rhs);
    };
};





class  Timer
{
friend class TimedTask;
public:
    Timer();
    virtual  ~Timer();

    //添加定时任务
    TimedTask::ptr AddTimedTask(std::function<void()>&& task,uint64_t ms,bool recycling=false);
    TimedTask::ptr AddTimedTask(const std::function<void()>& task,uint64_t ms,bool recycling=false);
    //添加定时任务附带条件，触发时会检查条件
     TimedTask::ptr AddTimedTask(std::function<void()>&& task,uint64_t ms,std::weak_ptr<void> condition,bool recycling=false);
    //获取小顶堆 堆顶定时任务距离现在还有多少ms触发
    uint64_t GetNextTaskInterval();
    //获取已经超时的定时任务
    void GetExpiredTasks(std::vector<std::function<void()>>&expired_tasks);
    //是否具有定时任务
    bool HasTimedTask();

protected:
    //添加定时器任务--辅助函数
    void AddTimedTask(TimedTask::ptr,std::lock_guard<std::mutex> &lock);
    //当插入的定时任务恰好在堆顶时，触发
    virtual void InsertAtFront()=0;

private:
    std::mutex mt;
    std::set<TimedTask::ptr,TimedTask::Comparator> tasks;
    bool trigger;   //是否触发InsertAtFront()
};
//************************************************************
//namespace  end
}