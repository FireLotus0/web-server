#include"timer.h"
#include"scheduler.h"
#include"macro.h"
#include"iomanager.h"
#include<sys/types.h>         
#include<sys/socket.h>
#include<string.h>
#include <arpa/inet.h>

static auto logger=GET_LOGGER("system");

void test()
{
    DEBUG(logger,"定时任务");
}

int main(int argc, const char** argv)
{
    HSQ::IOManager sc(2);
   
    int fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));

    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    inet_pton(AF_INET,"8.8.8.8",&addr.sin_addr.s_addr);

    sc.AddIOEvent(fd,HSQ::IOManager::IOEventType::WRITE,[](){std::cout<<"write  trigger"<<std::endl;});
    sc.AddTimedTask(test,2000);
    int rt=connect(fd,(sockaddr*)&addr,sizeof(addr));
    ASSERT(rt==0,"connect error");

    return 0;
}