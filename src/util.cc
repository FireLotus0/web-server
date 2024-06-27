#include "util.h"
#include "macro.h"
#include <chrono>
#include <iomanip>
#include <ctime>

namespace HSQ
{

//*************************************************************************
//获取线程ID
   unsigned long long GetThreadId()
    {
        std::stringstream ss;
        ss<<std::this_thread::get_id();
        unsigned long long id=std::stoull(ss.str());
        return id;
    }


//设置线程名:不能超过16个字符
    void SetThreadName(decltype(std::declval<std::thread>().native_handle()) handle,const std::string& name )
    {
        pthread_setname_np(handle,name.c_str());
    }
//获取线程名
    
    std::string GetThreadName()
    {
        char buf[16];
        prctl(PR_GET_NAME,buf);
        auto str=std::string(buf);
        //str.erase(remove_if(str.begin(), str.end(), ::isspace), str.end());
        return str;
    }


//*************************************************************************
 //封装用于读写锁的互斥量

    RWMutex::RWMutex()
    {
        pthread_rwlock_init(&m_lock,nullptr);
    }

    RWMutex::~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    void RWMutex::rdlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }

    void RWMutex::wrlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }

    void RWMutex::unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

//*************************************************************************
 /*使用原子操作实现自旋锁：自旋锁在等待时间不长的情况下，比使用互斥量性能高*/
void SpinLock::lock()
{
    while(flag.test_and_set(std::memory_order_acq_rel));
}
void SpinLock::unlock()
{
    flag.clear();
}
//避免忘记调用unlock()
SpinLock::~SpinLock()
{
        flag.clear();
}
//*************************************************************************
/*ElapsedTime成员函数*/
std::chrono::time_point<std::chrono::high_resolution_clock> ElapsedTime::start_point=std::chrono::high_resolution_clock::now();

std::string ElapsedTime::CurrentTimeStr()
{
	auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::stringstream ss;
	ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
	return ss.str();
}
uint64_t ElapsedTime::ElspsedMs()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now-start_point).count();
}
uint64_t ElapsedTime::ElapsedUs()
{
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now-start_point).count();
}

//*************************************************************************
//全局静态对象初始化顺序：
// InitProgram::InitProgram()
// {
//     auto logger=GET_LOGGER("system");
//     DEBUG(logger,"hello InitProgram()");
// }

// static InitProgram _Init;
//namespace end
}