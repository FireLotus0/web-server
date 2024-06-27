#pragma once

#include "tcpserver.h"
#include "httpsession.h"
#include "serverlet.h"

namespace HSQ {
namespace http {

/**
 * @brief HTTP服务器类
 */
class HttpServer : public TcpServer {
public:
    /// 智能指针类型
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false
               ,HSQ::IOManager* worker = HSQ::IOManager::GetThis()
               ,HSQ::IOManager* io_worker = HSQ::IOManager::GetThis()
               ,HSQ::IOManager* accept_worker = HSQ::IOManager::GetThis());

    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}

    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

    virtual void setName(const std::string& v) override;

protected:
    virtual void handleClient(Socket::ptr client) override;
private:
    /// 是否支持长连接
    bool m_isKeepalive;
    /// Servlet分发器
    ServletDispatch::ptr m_dispatch;
};

}
}
