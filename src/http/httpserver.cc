#include "httpserver.h"
#include "macro.h"

namespace HSQ {
namespace http {

static HSQ::Logger::ptr g_logger = GET_LOGGER("system");

HttpServer::HttpServer(bool keepalive
               ,HSQ::IOManager* worker
               ,HSQ::IOManager* io_worker
               ,HSQ::IOManager* accept_worker)
    :TcpServer(worker, io_worker, accept_worker)
    ,m_isKeepalive(keepalive) {
    m_dispatch.reset(new ServletDispatch);

    m_type = "http";
}

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>(v));
}

void HttpServer::handleClient(Socket::ptr client) {
    std::stringstream ss;
    std::string msg;

    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            ss << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive << 
                "客户端套接字是：" << client->getSocket() << " 监听套接字size:" <<
                m_socks.size() << " 监听套接字：" << m_socks[0]->getSocket() <<
                "当前协程："<<HSQ::Fiber::GetFiberId();
            msg = ss.str();
           // DEBUG(g_logger, msg);
            std::cout << msg << std::endl;

            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}
