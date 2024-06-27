#pragma once

#include<memory>
#include<vector>
#include <sys/types.h>  //使用fstat判断是否为socket： int fstat(int fd, struct stat *buf);
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include<sys/socket.h>
#include"util.h"
#include"macro.h"

/*管理文件句柄：
*文件句柄类型：是否为socket
*是否阻塞
*读写超时时间
*是否初始化
*当前状态：是否关闭
*/

namespace HSQ
{
//****************************************************

class Fd: public std::enable_shared_from_this<Fd>
{
public:
    using ptr=std::shared_ptr<Fd>;
    //通过文件句柄构造
    Fd(int fd);
    ~Fd();

    //状态读取
    bool IsInit();
    bool IsClose()const;
    bool IsSocket()const;
    bool IsSysNonBlock()const;
    bool IsUserNonBlock()const;
    //根据类型获取相应的超时时间：类型为SO_RCVTIMEO,O_SNDTIMEO
    uint64_t GetTimeOut(int type);

    //状态设置函数
    void SetUserNonBlock(bool flag);
    void SetSysNonBlock(bool flag);
    void SetTimeOut(int type,uint64_t ms);

    int getIntFd() const { return fd; }
private:
    //初始化FD
    bool init();
private:
    int fd;                 //文件句柄
    bool is_init:1;         //是否初始化
    bool is_close:1;        //是否关闭
    bool is_socket:1;       //是否为socket类型的句柄
    bool is_sysNonBlock:1;    //是否是通过hook设置非阻塞
    bool is_userNonBlock:1;   //是否已经设置了非阻塞
    uint64_t rcvtimeo;      //读超时时间,ms
    uint64_t sndtimeo;      //写超时时间,ms
};

//**********************************
//class FdManager:管理Fd对象，就像IOManager管理线程池一样
//是一个单例对象
class FdManager
{
public:
    typedef RWMutex RWMutexType;
    FdManager();

    //获取fd对象，如果该fd不存在，根据参数auto_create进行创建
    Fd::ptr GetFd(int fd,bool auto_create=false);
    //删除fd
    void DeleteFd(int fd);
private:
    RWMutexType m_mutex;
    std::vector<Fd::ptr> fd_vec;
};

//定义单例别名
using SingleFdManager=HSQ::Singleton<FdManager>;
//****************************************************
//namespace end
}