#pragma once


//Logger：输出器，一个logger对应多个appender，一个appender对应一个format
#include<string>
#include<list>
#include<thread>
#include<mutex>

#include"logevent.h"
#include"logappender.h"
#include"loglevel.h"
namespace HSQ
{

//一个logger在多线程环境中存在数据竞争
class Logger
{
public:
    using ptr=std::shared_ptr<Logger>;

    Logger();
    Logger(const std::string& name,HSQ::LogLevel::Level level);

    //一个输出函数，
    void log(HSQ::LogEvent::ptr);
    //一个获取level的函数
    HSQ::LogLevel::Level GetLevel()const;
    //一个获取用户名的函数
    std::string GetUserName()const;

    void AddAppender(HSQ::LogAppender::ptr);
    void DeleteAppender(HSQ::LogAppender::ptr);
    void ClearAppender();
    //默认构造函数会将标准输出添加到m_appenders中，如果不需要，可通过此函数重置
    void ResetAppender(HSQ::LogAppender::ptr appender);
private:
    std::string m_user_name = "system";
    HSQ::LogLevel::Level m_level = HSQ::LogLevel::Level::DEBUG;
    std::list<HSQ::LogAppender::ptr> m_appenders;
    mutable std::mutex mt;
};



//*************************************************************************
//namespace end
}
