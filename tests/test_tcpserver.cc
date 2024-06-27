#include "tcpserver.h"
#include "iomanager.h"
#include "macro.h"

HSQ::Logger::ptr g_logger = GET_LOGGER("root");

void run() {
    auto addr = HSQ::Address::LookupAnyAddress("0.0.0.0:8033");
    if(addr == nullptr) {
        ERROR(g_logger, "Look up rt is nullptr");
    } else {
        INFO(g_logger, addr->toString());
    }
    std::vector<HSQ::Address::ptr> addrs;
    addrs.push_back(addr);
    INFO(g_logger, "will bind");
    HSQ::TcpServer::ptr tcp_server(new HSQ::TcpServer);
    std::vector<HSQ::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        //sleep(2);
        INFO(g_logger, "bind error");

    }
    INFO(g_logger, "tcp will start");

    tcp_server->start();
    INFO(g_logger, "tcp run over");

}
int main(int argc, char** argv) {
    HSQ::IOManager iom(2);
    iom.Schedule(run);
    return 0;
}
