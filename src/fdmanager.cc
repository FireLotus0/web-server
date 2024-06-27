#include"fdmanager.h"


namespace HSQ
{

//日志调试
static auto logger=GET_LOGGER("system");

Fd::Fd(int f)
{
    fd=f;
    is_init=false;
    is_close=false;
    is_socket=false;
    is_sysNonBlock=false;
    is_userNonBlock=false;
    rcvtimeo=-1;    //0xffffffff
    sndtimeo=-1;    //0xffffffff
    init();
}
Fd::~Fd()
{

}

//状态读取

//IsInit：返回是否已经初始化，如果没有初始化，进行初始化
bool Fd::IsInit()
{
    return is_init; 
}
bool Fd::IsClose()const
{
    return is_close;
}
bool Fd::IsSocket()const
{
    return is_socket;
}
bool Fd::IsSysNonBlock()const
{
    return is_sysNonBlock;
}
bool Fd::IsUserNonBlock()const
{
    return is_userNonBlock;
}
//根据类型获取相应的超时时间：类型为SO_RCVTIMEO,O_SNDTIMEO
uint64_t Fd::GetTimeOut(int type)
{
    if(type==SO_RCVTIMEO)
        return rcvtimeo;
    else
        return sndtimeo;
}

//状态设置函数
void Fd::SetUserNonBlock(bool flag)
{
    is_userNonBlock=flag;
}
void Fd::SetSysNonBlock(bool flag)
{
    is_sysNonBlock=flag;
}
//设置超时时间
void Fd::SetTimeOut(int type,uint64_t ms)
{
    if(type==SO_RCVTIMEO)
        rcvtimeo=ms;
    else
        sndtimeo=ms;
}

//private:
//初始化FD
bool Fd::init()
{
    if(is_init)
        return true;
    //初始化
    rcvtimeo=-1;
    sndtimeo=-1;

    //判断fd类型是否为socket
    struct stat fd_stat;
    if(-1==fstat(fd,&fd_stat))
    {
        is_init=false;
        is_socket=false;
        ERROR(logger,"bool Fd::IsInit() : -1==fstat(fd,&fd_stat)");
    }
    else
    {
        is_init=true;
        is_socket=S_ISSOCK(fd_stat.st_mode);
    }
    //只对socket才设置其为非阻塞，其他文件描述符不设置
    if(is_socket)
    {
        //获取文件状态标记
        int file_states=fcntl(fd,F_GETFL,0);
        //如果不是非阻塞，则设置为非阻塞
        if(!(file_states & O_NONBLOCK))
            fcntl(fd,F_SETFL,O_NONBLOCK);
        is_sysNonBlock=true;
    }
    else    
        is_sysNonBlock=false;

    is_userNonBlock=false;
    is_close=false;
    return  is_init;
}

//****************************************
//class FdManager:
//public:
FdManager::FdManager()
{
    fd_vec.resize(64,nullptr);
}

//获取fd对象，如果该fd不存在，根据参数auto_create进行创建
Fd::ptr FdManager::GetFd(int fd,bool auto_create)
{
    //DEBUG(logger, "进入GetFd");

    if(fd<0)
        return nullptr;
    //DEBUG(logger, "FdManager中的互斥量被协程" + std::to_string(HSQ::Fiber::GetCurrentRunningFiber()->GetFiberId()) + "占据");
    HSQ::ReadLock lock(m_mutex);
    //DEBUG(logger, "FdManager中的互斥量被协程" + std::to_string(Fiber::GetCurrentRunningFiber()->GetFiberId()) + "释放");
    std::stringstream ss;
    std::string msg;
    //for(int i = 0; i < fd_vec.size(); i++) {
    //    ss << fd_vec[i]->getIntFd() << "  ";
    //}
    //msg = ss.str();
    //DEBUG(logger, msg);
    //如果文件描述符超过当前容量，根据auto_create创建
    if(fd>=(int)fd_vec.size())
    {   //fd不存在但是不创建
        if(auto_create==false) {
            lock.unlock();
            return nullptr; 
        }
    }
    else
    {   //fd存在，不为空 或 fd存在，且为空，但不创建
        if(fd_vec[fd]!=nullptr || auto_create==false) {
            lock.unlock();
            return fd_vec[fd];
        }
            
    }
    lock.unlock();
    //DEBUG(logger, "FdManager中的互斥量被协程" + std::to_string(HSQ::Fiber::GetCurrentRunningFiber()->GetFiberId()) + "占据");
    //DEBUG(logger, "FdManager中的互斥量被协程" + std::to_string(HSQ::Fiber::GetCurrentRunningFiber()->GetFiberId()) + "释放");
    //fd不存在，要创建
    //fd存在，为空，要创建
    
    Fd::ptr new_fd(new Fd(fd));
     HSQ::WriteLock lock2(m_mutex);

    if(fd>=fd_vec.size())
        fd_vec.resize(fd*1.5,nullptr);
    fd_vec[fd]=new_fd;
    return new_fd;
}
//删除fd
void FdManager::DeleteFd(int fd)
{
    //DEBUG(logger, "delete fd write lock");

    HSQ::WriteLock lock2(m_mutex);
    if(fd<=0 || fd>=fd_vec.size())
        return;
    fd_vec[fd].reset();
    //DEBUG(logger, "delete fd write unlock");

}




//****************************************************************
//namespace end    
}