#include"loglevel.h"


namespace HSQ
{

    std::string LogLevel::LevelToString(LogLevel::Level level)
    {
        switch(level)
        {
            case LogLevel::Level::DEBUG:  return "DEBUG";
            case LogLevel::Level::INFO:  return "INFO";
            case LogLevel::Level::WARN:  return "WARN";
            case LogLevel::Level::ERROR:  return "ERROR";
            case LogLevel::Level::FATAL:  return "FATAL";
            default:  return "UNDEFINE";
        }
    }

    LogLevel::Level LogLevel::StringToLevel(const std::string& str)
    {
        if(str=="DEBUG" || str=="debug")
            return LogLevel::Level::DEBUG;
        else if(str=="INFO" || str=="info")
            return LogLevel::Level::INFO;
        else if(str=="WARN" || str=="warn")
            return LogLevel::Level::INFO;
        else if(str=="ERROR" || str=="error")
            return LogLevel::Level::ERROR;
        else if(str=="FATAL" || str=="fatal")
            return LogLevel::Level::FATAL;
        else
            return LogLevel::Level::UNDEFINE;

    }
}