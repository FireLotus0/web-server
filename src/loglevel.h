#pragma once

#include<string>


//定义日志级别
namespace HSQ
{
    class LogLevel
    {
    public:
        //枚举值是一个编译期间常量
        enum Level
        {
            DEBUG=0,INFO=1,WARN=2,ERROR=4,FATAL=5,UNDEFINE=6
        };
        static std::string LevelToString(LogLevel::Level level);
        static LogLevel::Level StringToLevel(const std::string& str);
    };

}