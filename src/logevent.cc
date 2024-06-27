#include"logevent.h"
#include"util.h"

namespace HSQ
{


LogEvent::LogEvent(const std::string& user,HSQ::LogLevel::Level level, const std::string & file_name,uint32_t line,const std::string& thread_name,uint32_t fiber)
{
    m_time=HSQ::ElapsedTime::CurrentTimeStr();
    m_elapsed_ms=HSQ::ElapsedTime::ElspsedMs();
    m_user_name=user;
    m_level=level;
    m_thread_id=HSQ::GetThreadId();
    m_thread_name=thread_name;
    m_fiber_id=fiber;
    m_file_name=file_name;
    m_line=line;
}

    //获取数据成员
std::string LogEvent::GetEventTime()const
{
    return m_time;
}

std::string LogEvent::GetEventElapsedMs()const
{
    return std::to_string(m_elapsed_ms);
}

std::string LogEvent::GetEventUserName()const
{
    return m_user_name;
}

std::string LogEvent::GetEventFileName()const
{
    return m_file_name;
}

std::string LogEvent::GetEventLevel()const
{
    return HSQ::LogLevel::LevelToString(m_level);
}

std::string LogEvent::GetEventThreadId()const
{
    std::stringstream ss;
    ss<<m_thread_id;
    return ss.str();
}

std::string LogEvent::GetEventThreadName()const
{
    return m_thread_name;
}

std::string LogEvent::GetEventFiberId()const
{
    return std::to_string(m_fiber_id);
}

std::string LogEvent::GetEventMessage()const
{
    return m_message.str();
}

std::string LogEvent::GetEventLine()const
{
    return std::to_string(m_line);
}

std::stringstream& LogEvent::GetEventSS()
{
    return m_message;
}

}