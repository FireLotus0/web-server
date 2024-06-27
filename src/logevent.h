#pragma once

#include<string>
#include<memory>
#include<sstream>
#include<type_traits>
#include<thread>
#include"loglevel.h"
//日志事件类
namespace HSQ
{
class LogEvent
{
public:
    using ptr=std::shared_ptr<LogEvent>;
public:
    LogEvent(const std::string& user,HSQ::LogLevel::Level level,const std::string& file_name,uint32_t line,
            const std::string& thread_name,uint32_t fiber);
    
    //获取数据成员
    std::string GetEventTime()const;

    std::string GetEventElapsedMs()const;

    std::string GetEventUserName()const;

    std::string GetEventFileName()const;

    std::string GetEventLevel()const;

    std::string GetEventThreadId()const;

    std::string GetEventThreadName()const;

    std::string GetEventFiberId()const;

    std::string GetEventMessage()const;
    
    std::string GetEventLine()const;

    std::stringstream& GetEventSS();

private:
    std::string m_time;           //日志产生的时间
    uint64_t m_elapsed_ms;        //距离程序运行到现在所经过的ms数
    std::string m_user_name;      //用户名
    std::string m_file_name;
    HSQ::LogLevel::Level m_level; //事件级别，由用户的级别进行设置
    unsigned long long m_thread_id;    //线程号
    std::string m_thread_name;    //线程名
    uint32_t m_fiber_id;          //协程号
    std::stringstream m_message;  //日志内容   
    uint32_t m_line;
};

//*************************************************************************
//namespace end
}