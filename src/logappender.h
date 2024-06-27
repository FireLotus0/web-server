#pragma once

#include<memory>
#include<fstream>
#include<string>
#include<iostream>

#include"loglevel.h"
#include"util.h"
#include"logformat.h"

namespace HSQ
{

//日志输出地点：标准输出，文件
class LogAppender
{
public:
    using ptr=std::shared_ptr<LogAppender>;
public:
    //默认构造函数：创建默认级别，默认的输出格式
    LogAppender();  

    //获取level
    HSQ::LogLevel::Level GetAppenderLevel()const;
    //设置格式，加锁
    void SetAppenderLevel(HSQ::LogLevel::Level level);
    //获取格式
    std::shared_ptr<LogFormat>  GetAppenderFormat()const;
    //设置level,多线程环境中存在多个线程访问同一个appender，加锁
    void SetAppenderFormat(const std::string& pattern); 
    //输出函数：纯虚函数，加锁
    virtual void log(HSQ::LogEvent::ptr event)=0;

    virtual ~LogAppender(){}
protected:
    HSQ::LogLevel::Level m_level;  //用于实现将事件按级别进行输出，级别大于等于appender的事件才会进行输出
    HSQ::SpinLock spinlock; //操作文件，或者操作控制台，加锁
    std::shared_ptr<LogFormat> m_format;    //将事件传递给format类，由其负责解析并返回结果，appender进行输出

};
//******************************
//向标准控制台输出
class StdoutAppender:public LogAppender
{
public:
    
    StdoutAppender():LogAppender(){}
    void log(HSQ::LogEvent::ptr event)override;
};

//向文件输出输出
class FileAppender:public LogAppender
{
public:
    FileAppender(const std::string& file_name):LogAppender(),m_file_name(file_name)
    {}

    void log(HSQ::LogEvent::ptr event)override;

    //假设当前文件记录日志时，如果每一次调用都打开，关闭，将会很耗时间
    //在第一次打开之后，不必再关闭，在析构函数中进行关闭
    ~FileAppender()
    {
        if(stream.is_open())
            stream.close();
    }
private:
    std::string m_file_name;
    std::ofstream stream;

};

//*************************************************************************
//namespace end
}