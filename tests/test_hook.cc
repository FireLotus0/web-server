#include"hook.h"
#include"macro.h"
#include"iomanager.h"
#include<sys/types.h>         
#include<sys/socket.h>
#include<string.h>
#include <arpa/inet.h>


static auto logger=GET_LOGGER("system");

void test_sleep()
{
     HSQ::IOManager iom(1);
     iom.Schedule(
        [](){sleep(2);DEBUG(logger,"sleep  2");}
     );

         iom.Schedule(
        [](){sleep(3);DEBUG(logger,"sleep  3");}
     );
}

void test_connect()
{
     HSQ::IOManager sc(3);
    int fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));

    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    inet_pton(AF_INET,"104.193.88.123",&addr.sin_addr.s_addr);

    sc.AddIOEvent(fd,HSQ::IOManager::IOEventType::WRITE,[](){std::cout<<"write  trigger"<<std::endl;});
    int rt=connect(fd,(sockaddr*)&addr,sizeof(addr));
    ASSERT(rt==0,"connect error");
    sc.Stop();
}

int main(int argc,char** argv)
{
    
    //test_sleep();
   
    test_connect();
    
    return 0;
}
