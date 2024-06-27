#pragma once

#include<memory>
#include <vector>
#include <map>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<stdint.h>


//封装IP地址，方便c++调用
namespace HSQ
{

class IPAddress;

//抽象类
class Address
{
public:
    using ptr=std::shared_ptr<Address>;

    //获取地址对应得到协议族
    int getFamily() const;

public://静态成员函数

    //通过sockaddr*创建Address，失败返回nullptr
    static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

    //解析域名
    static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    //返回任意一个解析到的地址
    static Address::ptr LookupAnyAddress(const std::string& host, int family = AF_INET, int type = 0, int protocol = 0);

    //返回解析到的IP地址
    static IPAddress* LookupAnyIPAddress(const std::string& host,int family = AF_INET, int type = 0, int protocol = 0);

    //保存本机所有地址,multimap中：key---网卡名  val---pair<IP，Port>
    static bool getInterfaceAddresses(std::multimap<std::string ,std::pair<Address::ptr, uint32_t> >& result,int family = AF_INET);

    //获取指定网卡名的地址
    static bool getInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result,const std::string& iface, int family = AF_INET);

public://虚函数
    
    virtual ~Address() {}

    virtual const sockaddr* getAddr() const = 0;

    virtual sockaddr* getAddr() = 0;

    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& Insert(std::ostream& os) const = 0;

    std::string toString() const;

    bool operator<(const Address& rhs) const;

    bool operator==(const Address& rhs) const;

    bool operator!=(const Address& rhs) const;

};

//IP地址基类
class IPAddress:public Address
{
public:
    using ptr=std::shared_ptr<IPAddress>;

    static IPAddress::ptr Create(const char* addrss,uint16_t port=0);

    //广播地址
    virtual IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) = 0;
    //网段地址
    virtual IPAddress::ptr getNetAddress(uint32_t prefix_len) = 0;
    //子网掩码
    virtual IPAddress::ptr getSubnetMask(uint32_t prefix_len) = 0;

    virtual uint32_t getPort() const = 0;

    virtual void setPort(uint16_t v) = 0;

    virtual const sockaddr* getAddr() const = 0;

    virtual sockaddr* getAddr() = 0;

    virtual socklen_t getAddrLen() const = 0;

    virtual std::ostream& Insert(std::ostream& os) const = 0;

};

//IPv4地址
class IPv4Address:public IPAddress
{
public:
    using ptr=std::shared_ptr<IPv4Address>;

    static IPv4Address::ptr Create(const char* address, uint16_t port = 0);

    IPv4Address(const sockaddr_in& address);

    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& Insert(std::ostream& os) const override;

    IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr getNetAddress(uint32_t prefix_len) override;
    IPAddress::ptr getSubnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in ipv4_addr;
};
/**
 *IPv6地址
 */
class IPv6Address : public IPAddress 
{
public:
    typedef std::shared_ptr<IPv6Address> ptr;

    static IPv6Address::ptr Create(const char* address, uint16_t port = 0);

    IPv6Address();

    IPv6Address(const sockaddr_in6& address);

    IPv6Address(const uint8_t address[16], uint16_t port = 0);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    std::ostream& Insert(std::ostream& os) const override;

    IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr getNetAddress(uint32_t prefix_len) override;
    IPAddress::ptr getSubnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;
private:
    sockaddr_in6 ipv6_addr;
};

/**
 * @brief UnixSocket地址
 */
class UnixAddress : public Address {
public:
    typedef std::shared_ptr<UnixAddress> ptr;

 
    UnixAddress();


    UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
    void setAddrLen(uint32_t v);
    std::string getPath() const;
    std::ostream& Insert(std::ostream& os) const override;
private:
    sockaddr_un unix_addr;
    socklen_t addr_length;
};


class UnknownAddress : public Address {
public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
    socklen_t getAddrLen() const override;
   
    std::ostream& Insert(std::ostream& os) const override;
private:
    sockaddr unknown_addr;
  
};

std::ostream& operator<<(std::ostream& os, const Address& addr);






//*************************************************************************
//namespace end
}