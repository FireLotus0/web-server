#include "socket.h"
#include "iomanager.h"

static HSQ::Logger::ptr g_logger = GET_LOGGER("system");

void test_socket() {
   std::string msg;

   std::vector<HSQ::Address::ptr> vec;

   int res = HSQ::Address::Lookup(vec, "39.156.66.14", AF_INET, SOCK_STREAM);
   if(!res)
   {
       ERROR(g_logger,"lookup fail");
       return;
   }
   for(auto& v:vec)
   {
       INFO(g_logger,v->toString());
   }
   HSQ::IPAddress::ptr addrRes;
   for(auto& i : vec) {
       INFO(g_logger, i->toString());
       addrRes = std::dynamic_pointer_cast<HSQ::IPAddress>(i);
       if(addrRes) {
           break;
       }
   }

    
   HSQ::IPAddress::ptr addr(HSQ::Address::LookupAnyIPAddress("www.baidu.com"));
   if(addr) {
       msg = "get address: " + addr->toString();
       INFO(g_logger, msg);
       std::cout << "23: " << msg << std::endl;
   } else {
       ERROR(g_logger, "get address fail");
       std::cout << "26: " << msg << std::endl;
       return;
   }

    HSQ::Socket::ptr sock = HSQ::Socket::CreateTCP(addr);
    addr->setPort(80);
    msg = "addr=" + addr->toString();
    INFO(g_logger, msg);
    if(!sock->connect(addr)) {
       msg = "connect " + addr->toString() + " fail";
       ERROR(g_logger, msg); 
       return;
    } else {
       msg = "connect " + addr->toString() + " connected";
       INFO(g_logger, msg); 
    }

    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0) {
       msg = "send fail rt=" + rt;
       INFO(g_logger, msg); 
       return;
    }

    std::string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());

    if(rt <= 0) {
       msg = "recv fail rt=" + rt;
       INFO(g_logger, msg); 
       return;
    }

    buffs.resize(rt);
    INFO(g_logger, buffs);
}

void test2() {
   //std::string msg;

   //HSQ::IPAddress::ptr addr = HSQ::Address::LookupAnyIPAddress("www.baidu.com:80");
   //if(addr) {
   //    msg = "get address: " + addr->toString();
   //    INFO(g_logger, msg); 
   //} else {
   //    ERROR(g_logger,  "get address fail");
   //    return;
   //}

   //HSQ::Socket::ptr sock = HSQ::Socket::CreateTCP(addr);
   //if(!sock->connect(addr)) {
   //    msg = "connect " + addr->toString() + " fail";
   //    ERROR(g_logger, msg); 
   //    return;
   //} else {
   //    msg = "connect " + addr->toString() + " connected";
   //    INFO(g_logger, msg); 
   //}

    //uint64_t ts = HSQ::GetCurrentUS();
    //for(size_t i = 0; i < 10000000000ul; ++i) {
    //    if(int err = sock->getError()) {
    //        msg = "err=" + err + " errstr=" + strerror(err);
    //        INFO(g_logger, msg); 
    //        break;
    //    }

        //struct tcp_info tcp_info;
        //if(!sock->getOption(IPPROTO_TCP, TCP_INFO, tcp_info)) {
        //    INFO(g_logger) << "err";
        //    break;
        //}
        //if(tcp_info.tcpi_state != TCP_ESTABLISHED) {
        //    INFO(g_logger)
        //            << " state=" << (int)tcp_info.tcpi_state;
        //    break;
        //}
        //static int batch = 10000000;
        //if(i && (i % batch) == 0) {
        //    uint64_t ts2 = HSQ::GetCurrentUS();
        //    msg = "i=" + i + " used: " + ((ts2 - ts) * 1.0 / batch) + " us";
        //    INFO(g_logger, msg); 
        //    ts = ts2;
        //}
    //}
}

int main(int argc, char** argv) {
    HSQ::IOManager iom;
    iom.Schedule(test_socket);
    iom.Schedule(&test2);
//   HSQ::Scheduler sc(2);
  // sc.Schedule(&test_socket);
//   sc.Start();
//   sc.Stop();
   
   
   return 0;
}
