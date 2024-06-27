#include"address.h"
#include"macro.h"
#include<vector>
#include<map>
#include<sstream>
#include "iomanager.h"

static auto logger=GET_LOGGER("system");
void test_addr()
{
    std::vector<HSQ::Address::ptr> vec;
    bool rt=HSQ::Address::Lookup(vec,"www.baidu.com");
    if(!rt)
    {
        ERROR(logger,"lookup fail");
        return;
    }
    for(auto& v:vec)
    {
        INFO(logger,v->toString());
    }
}

void test_iface()
{
    std::multimap<std::string,std::pair<HSQ::Address::ptr,uint32_t>> res;
    std::stringstream ss;
    bool rt=HSQ::Address::getInterfaceAddresses(res);
     if(!rt)
    {
        ERROR(logger,"lookup fail");
        return;
    }
    for(auto & v:res)
    {
        ss<<v.first<<"-"<<v.second.first->toString()<<v.second.second<<std::endl;
        INFO(logger,ss.str());
    }
}

void test_ipv4()
{
    auto addr=HSQ::IPAddress::Create("127.0.0.8");
    if(addr)
     INFO(logger,"success");
}

int main(int argc, const char** argv)
{
    test_addr();
    test_iface();
    test_ipv4();
    HSQ::IOManager iom;
    iom.Schedule(test_addr);
    return 0;
}
