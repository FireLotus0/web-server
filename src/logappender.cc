#include"logappender.h"


namespace HSQ
{
    //默认构造函数：创建默认级别，默认的输出格式
LogAppender::LogAppender()
{
    m_level=HSQ::LogLevel::Level::DEBUG;  
    m_format.reset(new HSQ::LogFormat);
}  

//获取level：因为设置level时会加锁，所以读取时不用再加锁
HSQ::LogLevel::Level LogAppender::GetAppenderLevel()const
{
    return m_level;
}
//设置格式，加锁
void LogAppender::SetAppenderLevel(HSQ::LogLevel::Level level)
{
    spinlock.lock();
    m_level=level;
    spinlock.unlock();
}
//获取格式:这个函数要慎重，考虑，因为将数据成员的指针传递出去，失去了锁的保护，除非format里面也会加锁，
//后面看需求，是否删除这个函数
std::shared_ptr<LogFormat> LogAppender::GetAppenderFormat()const
{
    return m_format;
}
//设置level,多线程环境中存在多个线程访问同一个appender，加锁
void LogAppender::SetAppenderFormat(const std::string& pattern)
{
    spinlock.lock();
    //如果format已经存在，则将格式传递过去，否则，创建
    if(m_format)
        m_format->SetPattern(pattern);
    else
        m_format.reset(new LogFormat(pattern));
    spinlock.unlock();
}

//******************************
//向标准控制台输出
void StdoutAppender::log(HSQ::LogEvent::ptr event)
{
    spinlock.lock();
    std::cout<<m_format->format(event);
    spinlock.unlock();
}


//向文件输出输出
void FileAppender::log(HSQ::LogEvent::ptr event)
{
    spinlock.lock();
    if(!stream.is_open())
        stream.open(m_file_name);
    stream<<m_format->format(event);
    spinlock.unlock();
}

//*************************************************************************
//namespace  end
}