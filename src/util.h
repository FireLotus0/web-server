#pragma once

#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <sstream>
#include <type_traits>
#include <cstdint>
#include <algorithm>
#include <pthread.h>
#include <sys/prctl.h>
#include <byteswap.h>
#include <iostream>
#include <endian.h>
#include <iostream>
/*
*功能：
*获取线程ID
*自旋锁
*封装读写锁：std::shared_mutex似乎不能用
*单例模式
*获取时间：程序运行开始到现在所经过的时间
*工厂模式：生产日志格式中Item工厂
*打印变参辅助函数
*字节序相关函数
*计算一个数的二进制中有多少个1
*/

//命名空间：撼山拳---小说《剑来》中撼山拳谱之意
#include"logformat.h"

namespace  HSQ
{

    //*************************************************************************
    //获取线程ID
    unsigned long long GetThreadId();
    
    //设置线程名
    void SetThreadName(decltype(std::declval<std::thread>().native_handle()),const std::string& name );
    //获取线程名
    std::string GetThreadName();
    //*************************************************************************
    //封装用于读写锁的互斥量
    class RWMutex
    {
    public:
        RWMutex();

        ~RWMutex();

        void rdlock();
        void wrlock();

        void unlock();
    private:
        pthread_rwlock_t m_lock;
    };
    //封装读锁
    template<typename T>
    struct ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked=true;
        }

        ~ReadScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if(!m_locked)
            {
                m_mutex.rdlock();
                m_locked=true;
            }
        }

        void unlock()
        {   
            if(m_locked)
            {   
                m_mutex.unlock();
                m_locked=false;
            }   
        }   
    private:
        T& m_mutex;
        bool m_locked;
    };


    template<typename T>
    struct WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked=true;
        }

        ~WriteScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if(!m_locked)
            {
                m_mutex.wrlock();
                m_locked=true;
            }
        }

        void unlock()
        {   
            if(m_locked)
            {   
                m_mutex.unlock();
                m_locked=false;
            }   
        }   
    private:
        T& m_mutex;
        bool m_locked;
    };

    using ReadLock=ReadScopedLockImpl<RWMutex>;
    using WriteLock=WriteScopedLockImpl<RWMutex>;
    //*************************************************************************
    /*单例模式:唯一性（线程安全）--->使用互斥量加静态成员变量，或静态局部变量，或once_flag，或原子操作，
              全局性--->静态，static
    *其中原子操作性能相对最好，为学习不同的方法，此处实现上述几种，最终程序使用原子操作实现的单例
    *改进单例模式，通过可变参数模板和完美转发，可以为对象的构造函数传递参数
    */
    // //方法1：全局锁+静态成员变量
    // std::mutex mt_singleton;    
    // template<typename T>
    // class Singleton_mt
    // {
    // public:
    //     template<typename...Args>
    //     static void Instance(Args&&...args)
    //     {
    //         instance=std::make_shared<T>(new T(std::forward<Args>(args)...));
    //     }
    //     template<typename...Args>
    //     static std::shared_ptr<T> GetInstance(Args&&...args)
    //     {
    //         std::lock_guard<std::mutex> lock(mt_singleton);//性能很低，会导致线程串行化
    //         if(instance==nullptr)
    //             Instance(std::forward<Args>(args)...);
            
    //         return instance;    
    //     }

    // private:
    //     Singleton_mt()=default;
    //     virtual ~Singleton_mt()=default;
    //     Singleton_mt(const Singleton_mt<T>&)=delete;
    //     Singleton_mt<T>& operator=(const Singleton_mt<T>&)=delete;
    //     static std::shared_ptr<T> instance;
    // };
    // template<typename T>
    // std::shared_ptr<T> Singleton_mt<T>::instance=nullptr;   //饿汉式

    // //方法2：使用静态局部变量。c++11起，保证静态局部变量是线程安全的。静态局部变量对应饿汉式单例
    // template<typename T>
    // class Singleton
    // {
    // public:
    //     template<typename...Args>
    //     static std::shared_ptr<T> GetInstance(Args...args)
    //     {
    //         static std::shared_ptr<T> instance(new T(std::forward<Args>(args)...));
    //         return instance;
    //     }
    // private:
    //     Singleton()=default;
    //     virtual ~Singleton()=default;
    //     Singleton(const Singleton<T>&)=delete;
    //     Singleton<T>& operator=(const Singleton<T>&)=delete;
    // };

    // //方法3：使用once_flag方式
    // std::once_flag singleton_flag;
    // template<typename T>
    // class Singleton_flag
    // {
    // public:
    //     template<typename...Args>
    //     static void Instance(Args&&...args)
    //     {
    //         instance=std::make_shared<T>(new T(std::forward<Args>(args)...));
    //     }

    //     template<typename...Args>
    //     static std::shared_ptr<T> GetInstance(Args&&...args)
    //     {
    //         std::call_once(singleton_flag,&Singleton_flag::Instance,std::forward<Args>(args)...);
    //         return instance;
    //     }
    // private:
    //     Singleton_flag()=default;
    //     virtual ~Singleton_flag()=default;
    //     Singleton_flag(const Singleton_flag<T>&)=delete;
    //     Singleton_flag<T>& operator=(const Singleton_flag<T>&)=delete;

    //     static std::once_flag singleton_flag;
    //     static std::shared_ptr<T> instance;
    // };
    // template<typename T>
    // std::once_flag Singleton_flag<T>::singleton_flag;
    // template<typename T>
    // std::shared_ptr<T> Singleton_flag<T>::instance=nullptr;


    //方法4：使用原子操作---acquire-release  sematic。顺序一致性模型开销相对其他内存模型大，relaxed-sematic需要额外的同步机制
    //程序中使用acquire-release内存模型,折中选择。Singleton是程序中使用的版本。
    template<typename T>
    class Singleton
    {
    public:
        template<typename...Args>
        static T* GetInstance(Args&&...args)
        {
            auto old_val=instance.load(std::memory_order_acquire);
            if(old_val!=nullptr)
                return old_val;
            //跨平台考虑，部分硬件平台不支持在一个原子操作中完成比较和赋值操作
            //因此，使用compare_exchange_weak
            while(!instance.compare_exchange_weak(old_val,new T(std::forward<Args>(args)...),std::memory_order_acq_rel));
            return instance.load(std::memory_order_acquire);
        }
    private:
        Singleton()=default;
        virtual ~Singleton()=default;
        Singleton<T>& operator=(const Singleton<T>&)=delete;
        Singleton(const Singleton<T>&)=delete;
        
        static std::atomic<T*> instance;
    };
    template<typename T>
    std::atomic<T*> Singleton<T>::instance{nullptr};

    //*************************************************************************

    /*使用原子操作实现自旋锁：自旋锁在等待时间不长的情况下，比使用互斥量性能高*/
    class SpinLock
    {
    private:
        std::atomic_flag flag=ATOMIC_FLAG_INIT;
    public:
        void lock();
        void unlock();
        //避免忘记调用unlock()
        ~SpinLock();
    };

     //*************************************************************************
    /*流逝时间类：获取当前时间，获取程序运行时间，*/
    class ElapsedTime
    {
    public:
        static std::string CurrentTimeStr();
	    static uint64_t ElspsedMs();
	    static uint64_t ElapsedUs();
    private:
        static std::chrono::time_point<std::chrono::high_resolution_clock> start_point;
    };
    //*************************************************************************
    /*日志格式项工厂类：*/
    // template<typename T>
    // class FormatItemFactory
    // {
    // public:
    //     static std::shared_ptr<T> Create(char ch)
    //     {
    //         switch(ch)
    //         {
    //             case 'm': return std::make_shared<MessageItem>(new MessageItem());
    //             case 'p': return std::make_shared<LevelItem>(new LevelItem());
    //             case 'd': return std::make_shared<DateTimeItem>(new DateTimeItem());
    //             case 'c': return std::make_shared<UserNameItem>(new UserNameItem());
    //             case 'T': return std::make_shared<TabItem>(new TabItem());
    //             case 'r': return std::make_shared<ElapsedMsItem>(new ElapsedMsItem());
    //             case 'f': return std::make_shared<FileNameItem>(new FileNameItem());
    //             case 't': return std::make_shared<ThreadNameItem>(new ThreadNameItem());
    //             case 'N': return std::make_shared<ThreadIdItem>(new ThreadIdItem());
    //             case 'F': return std::make_shared<FiberIdItem>(new FiberIdItem());
    //             case 'l': return std::make_shared<LineItem>(new LineItem());
    //             case 'n': return std::make_shared<NewLineItem>(new NewLineItem());
    //             case '[': return std::make_shared<OtherItem>(new OtherItem('['));
    //             case ']': return std::make_shared<OtherItem>(new OtherItem(']'));
    //         }
    //     }

    // };
    //*************************************************************************
    //全局静态对象
    class InitProgram
    {
    public:
        InitProgram();
    };

    //*************************************************************************

    //*************************************************************************
    //字节序转换，系统调用内部使用移位运算和位运算

    #define HSQ_LITTLE_ENDIAN 1
    #define HSQ_BIG_ENDIAN 2


    template<typename T>
    typename std::enable_if<sizeof(T)==sizeof(uint64_t),T>::type byteswap(T val)
    {
        return (T)bswap_64((uint64_t)val);
    }
    template<typename T>
    typename std::enable_if<sizeof(T)==sizeof(uint32_t),T>::type byteswap(T val)
    {
        return (T)bswap_32((uint32_t)val);
    }
    template<typename T>
    typename std::enable_if<sizeof(T)==sizeof(uint16_t),T>::type byteswap(T val)
    {
        return (T)bswap_16((uint16_t)val);
    }
    template<typename T>
    typename std::enable_if<sizeof(T)==sizeof(uint8_t),T>::type byteswap(T val)
    {
        return val;
    }


    #if BYTE_ORDER==BIG_ENDIAN 
        #define HSQ_BYTE_ORDER HSQ_BIG_ENDIAN
    #else
        #define HSQ_BYTE_ORDER HSQ_LITTLE_ENDIAN
    #endif 

    #if HSQ_BYTE_ORDER==HSQ_BIG_ENDIAN  
        template<typename T>
        T byteswapOnLittleEndian(T val)
        {
            return val;
        }
        template<typename T>
        T byteswapOnBigEndian(T val)
        {
            return byteswap(val);
        }
    #else
        template<typename T>
        T byteswapOnLittleEndian(T val)
        {
            return byteswap(val);
        }
        template<typename T>
        T byteswapOnBigEndian(T val)
        {
            return val;
        }
    #endif

    //*************************************************************************
    //计算一个数中二进制表示中1的个数
    template<typename T>
    static uint32_t CountBits(T val)
    {
        uint32_t count=0;
        for(;val;++count)
            val &= val-1;
        return count;
    }

//*************************************************************************
//namespace end
}
