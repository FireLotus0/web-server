
#pragma once

#include "socketstream.h"
#include "http.h"

namespace HSQ {
namespace http {

class HttpSession : public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    HttpSession(Socket::ptr sock, bool owner = true);

    HttpRequest::ptr recvRequest();

    int sendResponse(HttpResponse::ptr rsp);
};

}
}
