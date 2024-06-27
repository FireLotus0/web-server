#pragma once

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include "address.h"
#include "iomanager.h"

namespace HSQ
{
class Socket : public std::enable_shared_from_this<Socket>
{
public:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(Socket&&) = delete;

public:
    using ptr = std::shared_ptr<Socket>;
    using weak_ptr = std::weak_ptr<Socket>;

    Socket(int family, int type, int protocol = 0);
    ~Socket();

    int64_t getSendTimeOut();
    void setSendTimeOut(int64_t sendTimeOut);

    int64_t getRecvTimeOut();
    void setRecvTimeOut(int64_t recvTimeOut);

    bool getOption(int level, int option, void* result, size_t* len);

    template<typename T>
    bool getOption(int level, int option, T& result) {
        size_t length = sizeof(T);
        return getOption(level, option, &result, &length);
    }

    bool setOption(int level, int option, void* result, size_t len);

    template<typename T>
    bool setOption(int level, int option, T& val) {
        return setOption(level, option, &val, sizeof(T));
    }

    Socket::ptr accept();

    bool init(int sock);


    bool bind(const Address::ptr addr);

    bool connect(const Address::ptr addr, uint64_t timeout = -1);
    bool listen(int backlog = SOMAXCONN);

    bool close();

    int send(const void* buffer, size_t length, int flags = 0);
    int send(const iovec* buffer, size_t length, int flags = 0);

    int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0);
    int sendTo(const iovec* buffer, size_t length, const Address::ptr to, int flags = 0);

    int recv(void* buffer, size_t length, int flags = 0);
    int recv(iovec* buffer, size_t length, int flags = 0);

    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0);
    int recvFrom(iovec* buffer, size_t length,Address::ptr from,  int flags = 0);

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();

    int getFamily() const { return m_family; }
    int getType() const { return m_type; }
    int getProtocol() const { return m_protocol; }

    int isConnected() const { return m_isConnected; }
    bool isValid() const;
    bool getError() const;

    std::ostream& dump(std::ostream& os) const;

    std::string toString() const;

    int getSocket() const { return m_sock;}

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();
    bool cancelAll();

    static Socket::ptr CreateTCP(HSQ::Address::ptr address);

private:
    void initSock();
    void newSock();

private:
    int m_sock;
    int m_family;
    int m_type;
    int m_protocol;
    bool m_isConnected;
    Address::ptr m_localAddress;
    Address::ptr m_remoteAddress;
};


std::ostream& operator<<(std::ostream& os, const Socket& addr);
}