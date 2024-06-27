#include"bytearray.h"
#include"macro.h"
#include<vector>
#include<iostream>
#include<typeinfo>
static auto logger=GET_LOGGER("system");

template<typename T>
void test_num(size_t len)
{
    std::vector<T> vec;
    for(size_t i=0;i<10;++i)
    {
        vec.push_back(static_cast<T>(rand())); 
    }
        
    HSQ::ByteArray::ptr ba(new HSQ::ByteArray(len));
    for(auto& v:vec)
        ba->Write(v,false);
    ba->SetPosition(0);
    T res{};
    for(auto&v:vec)
    {
        ba->Read(res,false);
        ASSERT(res==v,"res!=v!!!");
        DEBUG(logger,"读取数据："+std::to_string(res));
    }

}

void test_string(size_t len)
{
    std::vector<std::string> vec;
    for(size_t i=0;i<10;++i)
    {
        vec.push_back("hello_"+std::to_string(rand())); 
    }
        
    HSQ::ByteArray::ptr ba(new HSQ::ByteArray(len));
    for(auto& v:vec)
        ba->Write(v,false);
    ba->SetPosition(0);
    std::string res{};
    for(auto&v:vec)
    {
        ba->Read(res,false);
        ASSERT(res==v,"res!=v!!!");
        DEBUG(logger,"读取数据："+res);
    }

}

int  main(int argc, const char** argv) 
{
    DEBUG(logger,"main begin");
    test_num<uint32_t>(10);
    test_num<int32_t>(1);
    test_num<int64_t>(2);
    test_num<uint8_t>(2);
    test_num<uint64_t>(2);

    test_string(10);
    return 0;
}