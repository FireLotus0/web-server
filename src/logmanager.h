#pragma once

#include<map>
#include<string>
#include<memory>
#include<logger.h>
#include"loglevel.h"
//用于管理logger，LogManager应当是一个单例
namespace HSQ
{

class LogManager
{
public:
    using ptr=std::shared_ptr<LogManager>;

    //默认构造函数，创建默认appender
    LogManager();

    //如果name不存在，将会创建，默认级别DEBUG，默认Appender--标准输出
    HSQ::Logger::ptr GetLogger(const std::string& user_name);

    //删除logger
    void DeleteLogger(const std::string& user_name);

    //返回logger个数
    int GetLoggerCount();
private:
    //一个管理logger的集合：map,根据用户名查找
    std::map<std::string,HSQ::Logger::ptr> m_loggers{};
    //一个互斥量
    std::mutex mt; 
};


}