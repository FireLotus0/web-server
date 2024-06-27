#include "tcpserver.h"
#include "macro.h"

namespace HSQ {

static HSQ::Logger::ptr g_logger = GET_LOGGER("system");

TcpServer::TcpServer(HSQ::IOManager* worker,
                    HSQ::IOManager* io_worker,
                    HSQ::IOManager* accept_worker)
    :m_worker(worker)
    ,m_ioWorker(io_worker)
    ,m_acceptWorker(accept_worker)
    ,m_recvTimeout((uint64_t)(60 * 1000 * 2))
    ,m_name("HSQ/1.0.0")
    ,m_isStop(true) {
}

TcpServer::~TcpServer() {
    for(auto& i : m_socks) {
        i->close();
    }
    m_socks.clear();
}

bool TcpServer::bind(HSQ::Address::ptr addr) {
    ASSERT(addr != nullptr, "addr is null in bind")
    std::vector<Address::ptr> addrs;
    std::vector<Address::ptr> fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails) {
    std::stringstream ss;
    std::string msg;
    for(auto& addr : addrs) {
        
        Socket::ptr sock = Socket::CreateTCP(addr);
        ERROR(g_logger, "create tcp");

        if(!sock->bind(addr)) {
            ss << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            msg = ss.str();
            ERROR(g_logger, msg);
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            ss << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]" << "监听成功，fd=" << sock->getSocket();
            msg = ss.str();
            ERROR(g_logger, msg);
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    if(!fails.empty()) {
        m_socks.clear();
        return false;
    }

    for(auto& i : m_socks) {
        ss << "type=" << m_type
            << " name=" << m_name
            << " server bind success: " << *i;
        msg = ss.str();
        INFO(g_logger, msg); 
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock) {

    while(!m_isStop) {
        DEBUG(g_logger, "准备调用accept获取客户端套接字, 监听套接字是" + std::to_string(sock->getSocket()));
        Socket::ptr client = sock->accept();
        if(client) {
            DEBUG(g_logger, "已经获取客户端套接字,客户端套接字是：" + std::to_string(client->getSocket()));

            client->setRecvTimeOut(m_recvTimeout);
            m_ioWorker->Schedule(std::bind(&TcpServer::handleClient,
                        shared_from_this(), client));
        } else {
            std::stringstream ss;
            ss << "获取客户端套接字失败errno=" << errno
                << " errstr=" << strerror(errno);
            std::string msg = ss.str();
            ERROR(g_logger, msg) 
        }
       // sleep(5);
    }
    DEBUG(g_logger, "start accept quit");

}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->Schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
        INFO(g_logger, "监听套接字：" + std::to_string(sock->getSocket()));

    }
    INFO(g_logger, "start over");
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->Schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::handleClient(Socket::ptr client) {
    
    INFO(g_logger, "handleClient: ") ;
}

std::string TcpServer::toString(const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name 
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

}
