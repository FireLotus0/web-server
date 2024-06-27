#include"logger.h"


namespace HSQ
{

Logger::Logger()
{
    m_user_name = "system";
    m_level= HSQ::LogLevel::Level::DEBUG;
    m_appenders.emplace_back(std::move(new HSQ::StdoutAppender));
}
Logger::Logger(const std::string& name,HSQ::LogLevel::Level level)
{
    m_user_name=name;
    m_level=level;
    auto temp=HSQ::LogAppender::ptr(new HSQ::StdoutAppender);
    m_appenders.emplace_back(std::move(temp));
}

//一个输出函数，
void Logger::log(HSQ::LogEvent::ptr event)
{
    std::lock_guard<std::mutex> lock(mt);
    for(auto&& i:m_appenders)
    {
        if(HSQ::LogLevel::StringToLevel(event->GetEventLevel())>=m_level)
            i->log(event);
    }
    
}
//一个获取level的函数
HSQ::LogLevel::Level Logger::GetLevel()const
{
    std::lock_guard<std::mutex> lock(mt);
    return m_level;
}
//一个获取用户名的函数
std::string Logger::GetUserName()const
{
    std::lock_guard<std::mutex> lock(mt);
    return m_user_name;
}

void Logger::AddAppender(HSQ::LogAppender::ptr appender)
{
    std::lock_guard<std::mutex> lock(mt);
    m_appenders.push_back(appender);
    //std::cout<<"appender.size()="<<m_appenders.size()<<std::endl;
}
void Logger::DeleteAppender(HSQ::LogAppender::ptr appender)
{
    std::lock_guard<std::mutex> lock(mt);
    m_appenders.remove(appender);
}
void Logger::ClearAppender()
{
    std::lock_guard<std::mutex> lock(mt);
    m_appenders.clear();
}

void Logger::ResetAppender(HSQ::LogAppender::ptr appender)
{
    std::lock_guard<std::mutex> lock(mt);
    m_appenders.clear();
    m_appenders.emplace_back(std::move(appender));
}


//*************************************************************************
//namespace end
}