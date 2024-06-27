#include "socket.h"
#include "fdmanager.h"
#include "logger.h"

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


namespace HSQ{

static HSQ::Logger::ptr g_logger = GET_LOGGER("system");

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false)
{

}

Socket::~Socket() {
    ::close(m_sock);
}

int64_t Socket::getSendTimeOut() {
    auto ctx = SingleFdManager::GetInstance()->GetFd(m_sock);
    if(ctx) {
        return ctx->GetTimeOut(SO_SNDTIMEO);
    }
}

void Socket::setSendTimeOut(int64_t sendTimeOut) {
   // struct timeval tv(time_t(sendTimeOut / 1000), suseconds_t(sendTimeOut % 1000 * 1000));
   // setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeOut() {
    auto ctx = SingleFdManager::GetInstance()->GetFd(m_sock);
    if(ctx) {
        return ctx->GetTimeOut(SO_RCVTIMEO);
    }
    return -1;
}

void Socket::setRecvTimeOut(int64_t recvTimeOut) {
  // struct timeval tv(time_t(recvTimeOut / 1000), suseconds_t(recvTimeOut % 1000 * 1000));
  // setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}

bool Socket::getOption(int level, int option, void* result, size_t* len) {
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt) {
        return false;
        std::stringstream ss;
        ss << "getOption socket=" << m_sock << " level=" << level << " option=" << option 
           << " errno=" << errno << " strerror=" << strerror(errno);
        auto msg = ss.str();
        DEBUG(g_logger, msg);
        return false;
    }
    return true;
}

bool Socket::setOption(int level, int option, void* result, size_t len) {
    int rt;
    if(rt = setsockopt(m_sock, level, option, result, (socklen_t)len)) {
        std::stringstream ss;
        ss << "setOption socket=" << m_sock << " level=" << level << " option=" << option 
           << " errno=" << errno << " strerror=" << strerror(errno);
        auto msg = ss.str();

        DEBUG(g_logger, msg);
        return false;
    }
    return true;
}


Socket::ptr Socket::accept() {
   // DEBUG(g_logger, "进入accept");
    Socket::ptr sock(new Socket(PF_INET, SOCK_STREAM, IPPROTO_TCP));
   // DEBUG(g_logger, "创建了一个sock");
    int newSock = ::accept(m_sock, NULL, NULL);
   // DEBUG(g_logger, "调用全局的accept");

    if(newSock == -1) {
        std::stringstream ss;
        ss << "accept(" << m_sock << ") " 
           << " errno=" << errno << " strerror=" << strerror(errno);
        auto msg = ss.str();

        ERROR(g_logger, msg);
        return nullptr;
    }
    if(sock->init(newSock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::init(int sock) {
    auto ctx = SingleFdManager::GetInstance()->GetFd(sock);
    if(ctx && ctx->IsSocket() && !ctx->IsClose()) {
        m_sock = sock;
        m_isConnected = true;
        initSock();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}

bool Socket::bind(const Address::ptr addr) {
    if(!isValid()) {
        newSock();
        if(HSQ_UNLIKELY(!isValid())) {
            return false;
        }
    }
    std::stringstream ss;
    if(HSQ_UNLIKELY(addr->getFamily() != m_family)) {
        ss <<  "bind sock.family("   <<    m_family << ") addr.family(" << addr->getFamily()
                         << ") not equal, addr=" << addr->toString();
        auto msg = ss.str();

        ERROR(g_logger, msg);
        return false;
    }

    UnixAddress::ptr uaddr = std::dynamic_pointer_cast<UnixAddress>(addr);
    if(uaddr) {
        //Socket::ptr sock = Socket::CreateUnixTCPSocket();
        //if(sock->connect(uaddr)) {
        //    return false;
        //} else {
        //  //  HSQ::FSUtil::Unlink(uaddr->getPath(), true);
        //}
    }

    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        ss << "bind error errrno=" << errno
            << " errstr=" << strerror(errno);

        auto msg = ss.str();
        ERROR(g_logger, msg);
        return false;
    }
    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr, uint64_t timeout) {
    m_remoteAddress = addr;
    if(!isValid()) {
        newSock();
        if(HSQ_UNLIKELY(!isValid())) {
            return false;
        }
    }

    std::stringstream ss;

    if(HSQ_UNLIKELY(addr->getFamily() != m_family)) {
        ss << "connect sock.family("
            << m_family << ") addr.family(" << addr->getFamily()
            << ") not equal, addr=" << addr->toString();
        auto msg = ss.str();

        ERROR(g_logger, msg) ;
        return false;
    }

    if(timeout == (uint64_t)-1) {
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
            ss << "sock=" << m_sock << " connect(" << addr->toString()
                << ") error errno=" << errno << " errstr=" << strerror(errno);
            auto msg = ss.str();
            ERROR(g_logger, msg) ;
            close();
            return false;
        }
    } else {
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout)) {
            ss<< "sock=" << m_sock << " connect(" << addr->toString()
                << ") timeout=" << timeout << " error errno="
                << errno << " errstr=" << strerror(errno);
            auto msg = ss.str();
            ERROR(g_logger, msg); 
            close();
            return false;
        }
    }
    m_isConnected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}

Socket::ptr Socket::CreateTCP(HSQ::Address::ptr address) {
    ASSERT(address != nullptr, "address is null when create");
    Socket::ptr sock(new Socket(address->getFamily(), SOCK_STREAM, 0));
    return sock;
}

bool Socket::listen(int backlog) {
    if(!isValid()) {
        ERROR(g_logger, "listen error sock=-1");
        return false;
    }
    if(::listen(m_sock, backlog)) {
        std::stringstream ss;
        ss << "listen error errno=" << errno
            << " errstr=" << strerror(errno);
        auto msg = ss.str();

        ERROR(g_logger, msg);
        return false;
    }
    return true;
}

bool Socket::close() {
 if(!m_isConnected && m_sock == -1) {
        return true;
    }
    m_isConnected = false;
    if(m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

int Socket::send(const void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::send(const iovec* buffer, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags) {
     if(isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}

int Socket::sendTo(const iovec* buffer, size_t length, const Address::ptr to, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags) {
    if(isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recv(iovec* buffer, size_t length, int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags) {
    if(isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

int Socket::recvFrom(iovec* buffer, size_t length,Address::ptr from,  int flags) {
    if(isConnected()) {
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffer;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress() {
    if(m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)) {
        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if(m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch(m_family) {
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknownAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, result->getAddr(), &addrlen)) {
        std::stringstream ss;
        ss << "getsockname error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        auto msg = ss.str();

        ERROR(g_logger, msg); 

        return Address::ptr(new UnknownAddress(m_family));
    }
    if(m_family == AF_UNIX) {
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    m_localAddress = result;
    return m_localAddress;

}

bool Socket::isValid() const {
    return m_sock != -1;

}

bool Socket::getError() const {
    int error = 0;
    socklen_t len = sizeof(error);
    //if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len)) {
    //    error = errno;
    //}
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const {
    os << "[Socket sock=" << m_sock
       << " is_connected=" << m_isConnected
       << " family=" << m_family
       << " type=" << m_type
       << " protocol=" << m_protocol;
    if(m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if(m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::string Socket::toString() const {
    std::stringstream ss;
    dump(ss);
    return ss.str();
}


bool Socket::cancelRead() {
    return IOManager::GetThis()->CancelIOEvent(m_sock, HSQ::IOManager::READ);
}

bool Socket::cancelWrite() {
    return IOManager::GetThis()->CancelIOEvent(m_sock, HSQ::IOManager::WRITE);

}

bool Socket::cancelAccept() {
    return IOManager::GetThis()->CancelIOEvent(m_sock, HSQ::IOManager::READ);

}

bool Socket::cancelAll() {
    return IOManager::GetThis()->CancelAllIOEvent(m_sock);

}

void Socket::initSock() {
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR, val);
    if(m_type == SOCK_STREAM) {
        setOption(IPPROTO_TCP, O_NDELAY, val);
    }
}

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if(HSQ_LIKELY(m_sock != -1)) {
        initSock();
    } else {
        std::stringstream ss;
        ss <<  "socket(" << m_family
            << ", " << m_type << ", " << m_protocol << ") errno="
            << errno << " errstr=" << strerror(errno);
        auto msg = ss.str();

        ERROR(g_logger, msg); 
    }
}

std::ostream& operator<<(std::ostream& os, const Socket& sock) {
     return sock.dump(os);
}


}