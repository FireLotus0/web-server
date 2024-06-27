#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>
#include"macro.h"
#include"util.h"
#include"address.h"

namespace HSQ
{
//调试日志
static auto logger=GET_LOGGER("system");

template<class T>
static T CreateMask(uint32_t bits) 
{
    return (1 << (sizeof(T) * 8 - bits)) - 1;
}

//*************************************************************************
//*************************************地址基类成员函数
//获取地址对应得到协议族
int Address::getFamily() const
{
    return getAddr()->sa_family;
}
//通过sockaddr*创建Address，失败返回nullptr
Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen)
{
     if(addr == nullptr) {
        return nullptr;
    }

    Address::ptr result;
    switch(addr->sa_family)
     {
        case AF_INET:
            result.reset(new IPv4Address(*((const sockaddr_in*)addr)));
            break;
        case AF_INET6:
            result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
            break;
        default:
            result.reset(new UnknownAddress(*addr));
            break;
    }
    return result;
}

/*memchr:
*  功能：从buf所指内存区域的前count个字节查找字符ch。
*  说明：当第一次遇到字符ch时停止查找。如果成功，返回指向字符ch的指针；否则返回NULL。
*       
*getaddrinfo (const char *__restrict __name,
        const char *__restrict __service,
        const struct addrinfo *__restrict __req,
        struct addrinfo **__restrict __pai);      
* 参数：
*       node：表示主机名或 IP 地址字符串，用于指定要查询的主机名或 IP 地址。可以是一个具体的主机名，或者是一个 IPv4 或 IPv6 地址的字符串表示。可以为 NULL，表示查询本地主机的地址信息。
*       service：表示服务名或端口号字符串，用于指定要查询的服务名或端口号。可以是一个具体的服务名（如 "http"、"ftp" 等），或者是一个端口号的字符串表示（如 "80"、"443" 等）。
*       hints：一个指向 struct addrinfo 结构的指针，用于提供关于期望的返回结果类型的提示。可以通过设置 hints 结构的成员来指定期望的地址族、套接字类型、协议等。
*       res：一个指向 struct addrinfo 结构指针的指针，用于存储返回的地址信息链表。
*       函数返回值：
*       
*       如果调用成功，返回值为 0，并且 res 指向一个链表结构，其中每个节点都包含一个网络地址信息。
*       如果调用失败，返回值非零，表示错误代码，可以使用 gai_strerror 函数将错误代码转换为相应的错误消息字符串。
*       getaddrinfo 函数根据传入的主机名、服务名和提示参数，在系统的网络配置和命名服务中进行查询，获取与之相关的网络地址信息。该函数支持 IPv4 和 IPv6 地址，并且可以根据提供的提示参数进行筛选和过滤。
*       
*       通过遍历返回的链表，你可以获取各个匹配的网络地址信息，包括 IP 地址、端口号、协议族等。你可以使用这些信息来创建套接字并建立网络连接。
*/
//解析域名
bool Address::Lookup(std::vector<Address::ptr>& result, const std::string& host,int family , int type , int protocol )
{
    addrinfo hints, *results, *next;
    hints.ai_flags = 0;             //指示在getaddrinfo函数中使用的选项的标志
    hints.ai_family = family;       //地址协议族
    hints.ai_socktype = type;       //socket类型：SOCK_STREAM,SOCK_DGRAM
    hints.ai_protocol = protocol;   //协议类型：IPPROTO_TCP,IPPROTO_UDP
    hints.ai_addrlen = 0;           //指向的缓冲区的长度（以字节为单位）
    hints.ai_canonname = NULL;      //主机的规范名称
    hints.ai_addr = NULL;           //指向sockaddr结构的指针
    hints.ai_next = NULL;           //指向链表中下一个结构的指针。此参数在链接列表的最后一个addrinfo结构中设置为NULL。


    std::string node;   //域名或者IP地址----IP
    const char* service = NULL; //服务名，如http,ftp---Port
    //注意：node和service可能为空，但不会同时为空

    //检查 ipv6 address serivce----[xxxxx]:port
    if(!host.empty() && host[0] == '[') 
    {   //检查是否为IPv6地址
        const char* endipv6 = (const char*)memchr(host.c_str() + 1, ']', host.size() - 1);
        if(endipv6) {
            //TODO check out of range
            if(*(endipv6 + 1) == ':') {
                service = endipv6 + 2;
            }
            node = host.substr(1, endipv6 - host.c_str() - 1);
        }
    }
    //如果host是IPv6地址，node存储ipv6地址，service存储标识端口位置的地址
    //检查 node serivce----XXX:Port
    if(node.empty()) 
    {   //检查是否为IPv4地址
        service = (const char*)memchr(host.c_str(), ':', host.size());
        if(service) {
            if(!memchr(service + 1, ':', host.c_str() + host.size() - service - 1)) {
                node = host.substr(0, service - host.c_str());
                ++service;
            }
        }
    }

    //node为空，传入的host为域名，不是IPv4，IPv6地址
    if(node.empty()) {
        node = host;
    }
    //解析域名,results指向链表头结点
    int error = getaddrinfo(node.c_str(), service, &hints, &results);
    if(error) {
        std::stringstream ss;
        ss<< "Address::Lookup getaddress(" << host << ", "
            << family << ", " << type << ") err=" << error << " errstr="
            << gai_strerror(error);
        ERROR(logger,ss.str()); 
        return false;
    }

    next = results;
    while(next)
    {
        result.emplace_back(std::move(Create(next->ai_addr, (socklen_t)next->ai_addrlen)));
        std::stringstream ss;
        ss<< ((sockaddr_in*)next->ai_addr)->sin_addr.s_addr;
        DEBUG(logger,ss.str()); 
        next = next->ai_next;
    }

    freeaddrinfo(results);
    return !result.empty();
}


//返回任意一个解析到的地址即可
Address::ptr Address::LookupAnyAddress(const std::string& host,int family , int type, int protocol )
{
    std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) 
    {
        return result[0];
    }
    return nullptr;
}
//返回解析到的IP地址
IPAddress* Address::LookupAnyIPAddress(const std::string& host,int family , int type , int protocol )
{
     std::vector<Address::ptr> result;
    if(Lookup(result, host, family, type, protocol)) 
    {
        for(auto& i : result) {
            IPAddress::ptr v = std::dynamic_pointer_cast<IPAddress>(i);
            if(v) {
               // INFO(logger, v->toString());
                return v.get();
            }
        }
    }
    return nullptr;
}

/*

struct ifaddrs
{
    struct ifaddrs  *ifa_next;           指向链表中下一个struct ifaddr结构
    char            *ifa_name;           网络接口名
    unsigned int     ifa_flags;          网络接口标志
    struct sockaddr *ifa_addr;           指向一个包含网络地址的sockaddr结构
    struct sockaddr *ifa_netmask;        指向一个包含网络掩码的结构
    union 
    {
        struct sockaddr *ifu_broadaddr;  如果(ifa_flags&IFF_BROADCAST)有效，ifu_broadaddr指向一个包含广播地址的结构
                     广播地址 
        struct sockaddr *ifu_dstaddr;    如果(ifa_flags&IFF_POINTOPOINT)有效，ifu_dstaddr指向一个包含p2p目的地址的结构。
                     目标地址
    } ifa_ifu;

#define              ifa_broadaddr ifa_ifu.ifu_broadaddr
#define              ifa_dstaddr   ifa_ifu.ifu_dstaddr
    void            *ifa_data;          指向一个缓冲区，其中包含地址族私有数据。没有私有数据则为NULL。
};

getifaddrs创建一个链表，链表上的每个节点都是一个struct ifaddrs结构，getifaddrs()返回链表第一个元素的指针
使用该函数，可以轻松实现TCP/IP服务器和客户端套接字列表、
获取网络接口信息等。在使用getifaddrs时，必须注意避免内存泄漏和缓冲区溢出引起的崩溃问题。
*/
//保存本机所有地址
bool Address::getInterfaceAddresses(std::multimap<std::string,std::pair<Address::ptr, std::uint32_t> >& result,int family )
{
    struct ifaddrs *next, *results;
    if(getifaddrs(&results) != 0) 
    {
        std::stringstream ss;
        ss<<"Address::getInterfaceAddresses getifaddrs   err=" << errno << " errstr=" << strerror(errno);
        ERROR(logger,ss.str()) ;
        return false;
    }

    try {
        for(next = results; next; next = next->ifa_next)
        {
            Address::ptr addr;
            std::uint32_t prefix_len = ~0u;
            // AF_UNSPEC   IPv4 IPv6均可
            if(family != AF_UNSPEC && family != next->ifa_addr->sa_family) 
            {
                continue;
            }
            switch(next->ifa_addr->sa_family) 
            {
                case AF_INET:
                    {
                        addr = Create(next->ifa_addr, (socklen_t)sizeof(sockaddr_in));
                        std::uint32_t netmask = ((sockaddr_in*)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len = CountBits(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr = Create(next->ifa_addr, sizeof(sockaddr_in6));
                        in6_addr& netmask = ((sockaddr_in6*)next->ifa_netmask)->sin6_addr;
                        prefix_len = 0;
                        for(int i = 0; i < 16; ++i) {
                            prefix_len += CountBits(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
            }

            if(addr) 
            {
                result.insert(std::make_pair(next->ifa_name,std::make_pair(addr, prefix_len)));
            }
        }
    } catch (...) {
        ERROR(logger,"Address::getInterfaceAddresses exception") ;
        freeifaddrs(results);
        return false;
    }
    freeifaddrs(results);
    return !result.empty();
}
//获取指定网卡名的地址
bool Address::getInterfaceAddresses(std::vector<std::pair<Address::ptr, std::uint32_t> >&result,const std::string& iface, int family )
{
    if(iface.empty() || iface == "*") 
    {
        if(family == AF_INET || family == AF_UNSPEC) 
        {
            result.push_back(std::make_pair(Address::ptr(new IPv4Address()), 0u));
        }
        if(family == AF_INET6 || family == AF_UNSPEC)
        {
            result.push_back(std::make_pair(Address::ptr(new IPv6Address()), 0u));
        }
        return true;
    }

    std::multimap<std::string,std::pair<Address::ptr, std::uint32_t> > results;

    if(!getInterfaceAddresses(results, family)) 
    {
        return false;
    }

    auto its = results.equal_range(iface);
    for(; its.first != its.second; ++its.first) {
        result.push_back(its.first->second);
    }
    return !result.empty();
}
std::string Address::toString() const
{
    std::stringstream ss;
    Insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const
{
 socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if(result < 0) {
        return true;
    } else if(result > 0) {
        return false;
    } else if(getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool Address::operator==(const Address& rhs) const
{
    return getAddrLen() == rhs.getAddrLen()  && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const
{
     return !(*this == rhs);
}


//*************************************IP地址基类成员函数

IPAddress::ptr IPAddress::Create(const char* address,uint16_t port)
{
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(addrinfo));

    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(address, NULL, &hints, &results);
    if(error)
    {
        std::stringstream ss;
        ss<<"IPAddress::Create(" << address
            << ", " << port << ") error=" << error
            << " errno=" << errno << " errstr=" << strerror(errno);
        ERROR(logger,ss.str()); 
        return nullptr;
    }

    try {
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
                Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result) {
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch (...) {
        freeaddrinfo(results);
        return nullptr;
    }
}


//*************************************IPv4地址成员函数
IPv4Address::ptr IPv4Address::Create(const char* address, std::uint16_t port)
{
    IPv4Address::ptr rt(new IPv4Address);
    rt->ipv4_addr.sin_port = HSQ::byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->ipv4_addr.sin_addr);
    if(result <= 0) 
    {
        std::stringstream ss;
        ss<< "IPv4Address::Create(" << address << ", "
                << port << ") rt=" << result << " errno=" << errno
                << " errstr=" << strerror(errno);
        ERROR(logger,ss.str());
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address)
{
    ipv4_addr=address;
}

IPv4Address::IPv4Address(uint32_t address , std::uint16_t port)
{
    memset(&ipv4_addr, 0, sizeof(ipv4_addr));
    ipv4_addr.sin_family = AF_INET;
    ipv4_addr.sin_port = HSQ::byteswapOnLittleEndian(port);
    ipv4_addr.sin_addr.s_addr = HSQ::byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address::getAddr() const 
{
    return (sockaddr*)&ipv4_addr;
}
sockaddr* IPv4Address::getAddr()
{
    return (sockaddr*)&ipv4_addr;
}
socklen_t IPv4Address::getAddrLen() const
{
    return sizeof(ipv4_addr);
}
std::ostream& IPv4Address::Insert(std::ostream& os) const 
{
    uint32_t addr = HSQ::byteswapOnLittleEndian(ipv4_addr.sin_addr.s_addr);
    os << ((addr >> 24) & 0xff) << "."
       << ((addr >> 16) & 0xff) << "."
       << ((addr >> 8) & 0xff) << "."
       << (addr & 0xff);
    os << ":" << HSQ::byteswapOnLittleEndian(ipv4_addr.sin_port);
    return os;
}


IPAddress::ptr IPv4Address::getBroadcastAddress(uint32_t prefix_len) 
{
    if(prefix_len > 32) 
        return nullptr;
    sockaddr_in baddr(ipv4_addr);
    baddr.sin_addr.s_addr |= HSQ::byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::getNetAddress(uint32_t prefix_len) 
{
    if(prefix_len > 32) 
        return nullptr;
    sockaddr_in baddr(ipv4_addr);
    baddr.sin_addr.s_addr &= byteswapOnLittleEndian(
            CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(baddr));
}
IPAddress::ptr IPv4Address::getSubnetMask(uint32_t prefix_len) 
{
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin_family = AF_INET;
    subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}
uint32_t IPv4Address::getPort() const
{
     return byteswapOnLittleEndian(ipv4_addr.sin_port);
}
void IPv4Address::setPort(std::uint16_t v) 
{
     ipv4_addr.sin_port = byteswapOnLittleEndian(v);
}


//*************************************IPv6成员函数
IPv6Address::ptr IPv6Address::Create(const char* address, std::uint16_t port)
{
    IPv6Address::ptr rt(new IPv6Address);
    rt->ipv6_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->ipv6_addr.sin6_addr);
    if(result <= 0) 
    {
        std::stringstream ss;
        ss<<"IPv6Address::Create(" << address << ", "<< port << ") rt=" << result << " errno=" << errno << " errstr=" << strerror(errno);
        ERROR(logger,ss.str()) ;
        return nullptr;
    }
    return rt;
}


IPv6Address::IPv6Address()
{
    memset(&ipv6_addr, 0, sizeof(ipv6_addr));
    ipv6_addr.sin6_family = AF_INET6;
}


IPv6Address::IPv6Address(const sockaddr_in6& address)
{
    ipv6_addr = address;
}



IPv6Address::IPv6Address(const uint8_t address[16], uint16_t port )
{
    memset(&ipv6_addr, 0, sizeof(ipv6_addr));
    ipv6_addr.sin6_family = AF_INET6;
    ipv6_addr.sin6_port = byteswapOnLittleEndian(port);
    memcpy(&ipv6_addr.sin6_addr.s6_addr, address, 16);
}

const sockaddr* IPv6Address::getAddr() const 
{
    return (sockaddr*)&ipv6_addr;
}
sockaddr* IPv6Address::getAddr()
{
    return (sockaddr*)&ipv6_addr;
}
socklen_t IPv6Address::getAddrLen() const 
{
    return sizeof(ipv6_addr);
}
std::ostream& IPv6Address::Insert(std::ostream& os) const 
{
    os << "[";
    uint16_t* addr = (uint16_t*)ipv6_addr.sin6_addr.s6_addr;
    bool used_zeros = false;
    for(size_t i = 0; i < 8; ++i) {
        if(addr[i] == 0 && !used_zeros) {
            continue;
        }
        if(i && addr[i - 1] == 0 && !used_zeros) {
            os << ":";
            used_zeros = true;
        }
        if(i) {
            os << ":";
        }
        os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
    }

    if(!used_zeros && addr[7] == 0) {
        os << "::";
    }

    os << "]:" << byteswapOnLittleEndian(ipv6_addr.sin6_port);
    return os;
}

IPAddress::ptr IPv6Address::getBroadcastAddress(uint32_t prefix_len) 
{
    sockaddr_in6 baddr(ipv6_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] |=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::getNetAddress(uint32_t prefix_len) 
{
    sockaddr_in6 baddr(ipv6_addr);
    baddr.sin6_addr.s6_addr[prefix_len / 8] &=
        CreateMask<uint8_t>(prefix_len % 8);
    for(int i = prefix_len / 8 + 1; i < 16; ++i) {
        baddr.sin6_addr.s6_addr[i] = 0x00;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address::getSubnetMask(uint32_t prefix_len) 
{
    sockaddr_in6 subnet;
    memset(&subnet, 0, sizeof(subnet));
    subnet.sin6_family = AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len /8] =
        ~CreateMask<uint8_t>(prefix_len % 8);

    for(uint32_t i = 0; i < prefix_len / 8; ++i) {
        subnet.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));
}
uint32_t IPv6Address::getPort() const 
{
    return byteswapOnLittleEndian(ipv6_addr.sin6_port);
}
void IPv6Address::setPort(uint16_t v) 
{
     ipv6_addr.sin6_port = byteswapOnLittleEndian(v);

}


//*************************************Unix地址

constexpr uint16_t MAX_PATH_LEN=255;
UnixAddress::UnixAddress()
{
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    addr_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}


UnixAddress::UnixAddress(const std::string& path)
{
    memset(&unix_addr, 0, sizeof(unix_addr));
    unix_addr.sun_family = AF_UNIX;
    addr_length = path.size() + 1;

    if(!path.empty() && path[0] == '\0') {
        --addr_length;
    }

    if(addr_length > sizeof(unix_addr.sun_path)) {
        throw std::logic_error("path too long");
    }
    memcpy(unix_addr.sun_path, path.c_str(), addr_length);
    addr_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr* UnixAddress::getAddr() const
{
    return (sockaddr*)&unix_addr;
}
sockaddr* UnixAddress::getAddr() 
{
     return (sockaddr*)&unix_addr;
}
socklen_t UnixAddress::getAddrLen() const
{
    return addr_length;
}

void UnixAddress::setAddrLen(uint32_t v)
{
    addr_length = v;
}
std::string UnixAddress::getPath()const
{
    std::stringstream ss;
    if(addr_length > offsetof(sockaddr_un, sun_path)
            && unix_addr.sun_path[0] == '\0') {
        ss << "\\0" << std::string(unix_addr.sun_path + 1,
                addr_length - offsetof(sockaddr_un, sun_path) - 1);
    } else {
        ss << unix_addr.sun_path;
    }
    return ss.str();
}
std::ostream& UnixAddress::Insert(std::ostream& os) const
{
     if(addr_length > offsetof(sockaddr_un, sun_path)
            && unix_addr.sun_path[0] == '\0') {
        return os << "\\0" << std::string(unix_addr.sun_path + 1,
                addr_length - offsetof(sockaddr_un, sun_path) - 1);
    }
    return os << unix_addr.sun_path;
}

//*************************************未知地址
UnknownAddress::UnknownAddress(int family)
{
    memset(&unknown_addr, 0, sizeof(unknown_addr));
    unknown_addr.sa_family = family;
}
UnknownAddress::UnknownAddress(const sockaddr& addr)
{
    unknown_addr = addr;
}
const sockaddr* UnknownAddress::getAddr() const 
{
    return (sockaddr*)&unknown_addr;
}
sockaddr* UnknownAddress::getAddr() 
{
    return (sockaddr*)&unknown_addr;
}
socklen_t UnknownAddress::getAddrLen() const 
{
    return sizeof(unknown_addr);
}
std::ostream& UnknownAddress::Insert(std::ostream& os) const
{
      os << "[UnknownAddress family=" << unknown_addr.sa_family << "]";
    return os;
}


//*************************************辅助输出函数
std::ostream& operator<<(std::ostream& os, const Address& addr)
{
    return addr.Insert(os);
}
//*************************************************************************
//namespace end
}

/*****************************************************************************************知识点
 * ai_flags：
Value Meaning
AI_PASSIVE 套接字地址将用于调用bind 函数
AI_CANONNAME 返回规范名称
AI_NUMERICHOST 传递给getaddrinfo函数的nodename参数必须是数字字符串。
AI_ALL If this bit is set, a request is made for IPv6 addresses and IPv4 addresses with AI_V4MAPPED.
AI_ADDRCONFIG 只有配置了全局地址后，getaddrinfo才会解析。 IPv6和IPv4环回地址不被认为是有效的全局地址。
AI_V4MAPPED 如果对IPv6地址的getaddrinfo请求失败，则对IPv4地址进行名称服务请求，这些地址将转换为IPv4映射IPv6地址格式。
AI_NON_AUTHORITATIVE 地址信息可以来自非授权命名空间提供商
AI_SECURE 地址信息来自安全信道。
AI_RETURN_PREFERRED_NAMES 地址信息是用于用户的优选名称。
AI_FQDN getaddrinfo将返回名称最终解析为的完全限定域名。 完全限定域名在ai_canonname成员中返回。
这与AI_CANONNAME位标记不同，后者返回在DNS中注册的规范名称，该名称可能与平面名称解析为的完全限定域名不同。
只能设置AI_FQDN和AI_CANONNAME位中的一个。 如果EAI_BADFLAGS同时存在这两个标志，getaddrinfo函数将失败。
AI_FILESERVER 命名空间提供程序提示正在查询的主机名正在文件共享方案中使用。 命名空间提供程序可以忽略此提示。

ai_family: The address family.

AF_UNSPEC 地址系列未指定。
AF_INET IPv4 address family.
AF_NETBIOS NetBIOS地址系列。
AF_INET6 IPv6 address family.
AF_IRDA The Infrared Data Association address family.
AF_BTH Bluetooth address family.

ai_protocol: 协议类型。

Value Meaning
IPPROTO_TCP 传输控制协议（TCP）。 当ai_family成员为AF_INET或AF_INET6且ai_socktype成员为SOCK_STREAM时，这是一个可能的值
IPPROTO_UDP 用户数据报协议（UDP）。 当ai_family成员为AF_INET或AF_INET6且类型参数为SOCK_DGRAM时，这是一个可能的值。
IPPROTO_RM PGM协议用于可靠的组播。 当ai_family成员为AF_INET且ai_socktype成员为SOCK_RDM时，这是一个可能的值。 在为Windows Vista及更高版本发布的Windows SDK上，此值也称为IPPROTO_PGM。
可能的选项特定于指定的地址系列和套接字类型。
如果为ai_protocol指定了值0，则调用者不希望指定协议，服务提供者将选择要使用的ai_protocol。 对于IPv4和IPv6之外的协议，将ai_protocol设置为零。
下表列出了ai_protocol成员的通用值，尽管其他许多值也是可能的。

ai_socktype:　　套接字类型

Value Meaning
SOCK_STREAM 使用OOB数据传输机制提供顺序，可靠，双向，基于连接的字节流。使用Internet地址系列（AF_INET或AF_INET6）的传输控制协议（TCP）。如果ai_family成员是AF_IRDA，则SOCK_STREAM是唯一支持的套接字类型。
SOCK_DGRAM 支持数据报，它是无连接的，不可靠的固定（通常小）最大长度的缓冲区。对Internet地址系列（AF_INET或AF_INET6）使用用户数据报协议（UDP）。
SOCK_RAW 提供一个原始套接字，允许应用程序处理下一个上层协议头。要操作IPv4标头，必须在套接字上设置IP_HDRINCL套接字选项。要操作IPv6头，必须在套接字上设置IPV6_HDRINCL套接字选项。
SOCK_RDM 提供可靠的消息数据报。这种类型的示例是在Windows中的实用通用多播（PGM）多播协议实现，通常被称为可靠多播节目。
SOCK_SEQPACKET 基于数据报提供伪流包。
 
*/