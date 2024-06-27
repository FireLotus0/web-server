#include"logmanager.h"
#include<iostream>
namespace HSQ
{
    LogManager::LogManager()
    {   //LogManager将会是一个单例对象，所以构造是线程安全的
        std::shared_ptr<Logger> logger(new HSQ::Logger("system",HSQ::LogLevel::Level::DEBUG));
        m_loggers.emplace_hint(m_loggers.begin(),"system",logger);
    }

    //如果name不存在，将会创建，默认级别DEBUG，默认Appender--标准输出
    HSQ::Logger::ptr LogManager::GetLogger(const std::string& user_name)
    {
        std::lock_guard<std::mutex> lock(mt);
        auto iter=m_loggers.find(user_name);
        if(iter==m_loggers.end())
        {
            HSQ::Logger::ptr logger(new HSQ::Logger(user_name,HSQ::LogLevel::Level::DEBUG));
            m_loggers.insert({user_name,logger});
            return logger;           
        }
        return iter->second;
    }

    //删除logger
    void LogManager::DeleteLogger(const std::string& user_name)
    {
        std::lock_guard<std::mutex> lock(mt);
        auto iter=m_loggers.find(user_name);
        if(iter!=m_loggers.end())
        {
            iter->second.reset();
            m_loggers.erase(user_name);
        }
        
    }

    int  LogManager::GetLoggerCount()
    {
        return m_loggers.size();
    }
}