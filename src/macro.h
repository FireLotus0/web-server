#pragma once
/*定义程序中用到的宏*/
//LogEvent(const std::string& user,HSQ::LogLevel::Level level,const std::string& file_name,
//         uint32_t line,const std::string& thread_name,uint32_t fiber);
#include"util.h"
#include"logmanager.h"
#include"fiber.h"

//__builtin_except(!!(x), 1)
//__builtin_except(!!(x), 0)
#if defined __GNUC__ || defined __LLVM__
        #define HSQ_LIKELY(x) (x)
        #define HSQ_UNLIKELY(x)  (x)
#else
        #define HSQ_LIKELY(x)  (x)
        #define HSQ_UNLIKELY(x) (x)
#endif

//输出日志,eg:DEBUG(logger,"输出内容")
#define LOG(logger) \
        logger->log(a_log_event);
#define XX(logger,str,level) \
        { \
        HSQ::LogEvent::ptr a_log_event(new HSQ::LogEvent(logger->GetUserName(), \
                HSQ::LogLevel::Level::level,__FILE__,__LINE__,"main",HSQ::Fiber::GetFiberId()));\
                a_log_event->GetEventSS()<<str; \
        LOG(logger) \
        }
#define DEBUG(logger,str)  XX(logger,str, DEBUG)
#define INFO(logger,str)  XX(logger,str, INFO)
#define INFO(logger,str)  XX(logger,str, INFO)
#define WARN(logger,str)  XX(logger,str, WARN)
#define ERROR(logger,str)  XX(logger,str, ERROR)
#define FATAL(logger,str)  XX(logger,str, FATAL)

//获取logger
#define GET_LOGGER(user_name)\
        HSQ::Singleton<HSQ::LogManager>::GetInstance()->GetLogger(user_name)

//断言
#define ASSERT(condition,message) \
        if(!(condition)) {\
                ERROR(GET_LOGGER("system"),(message)) \
                 \
        }


