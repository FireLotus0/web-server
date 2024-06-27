#include "httpserver.h"
#include "macro.h"
#include "db.h"
#include <iostream>

static HSQ::Logger::ptr g_logger = GET_LOGGER("server");

#define TOSTRING(...) #__VA_ARGS__


HSQ::IOManager::ptr worker;
void run() {

    HSQ::http::HttpServer::ptr server(new HSQ::http::HttpServer(true, worker.get(), HSQ::IOManager::GetThis()));
    //HSQ::http::HttpServer::ptr server(new HSQ::http::HttpServer(true));
    HSQ::Address::ptr addr = HSQ::IPv4Address::Create("0.0.0.0", 8020);//(HSQ::Address::LookupAnyIPAddress("127.0.0.1:8020"));
    if(addr == nullptr) {
        std::cout << "addr is null" <<std::endl;
    } else {
        std::cout << addr->toString() <<std::endl;
    }
    while(!server->bind(addr)) {
       sleep(2);
    }
    //INFO(g_logger, "addr is " + addr->toString());
    std::cout << "addr is " << addr->toString() << std::endl;
    auto sd = server->getServletDispatch();
    sd->addServlet("/HSQ/xx", [](HSQ::http::HttpRequest::ptr req
                ,HSQ::http::HttpResponse::ptr rsp
                ,HSQ::http::HttpSession::ptr session) {
            
            HSQ::SqlDb db;
            db.openDb("test.db");
            auto p = db.query("SELECT * from TestTable");
            rapidjson::Document& doc = *p;
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
            const std::string jsonString = buffer.GetString();
            std::cout << jsonString;
            rsp->setBody(jsonString);
            return 0;
    });

    sd->addGlobServlet("/HSQ/*", [](HSQ::http::HttpRequest::ptr req
                ,HSQ::http::HttpResponse::ptr rsp
                ,HSQ::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/HSQx/*", [](HSQ::http::HttpRequest::ptr req
                ,HSQ::http::HttpResponse::ptr rsp
                ,HSQ::http::HttpSession::ptr session) {
            rsp->setBody(TOSTRING(<html>
                            <head><title>404 Not Found</title></head>
                            <body>
                            <center><h1>404 Not Found</h1></center>
                            <hr><center>nginx/1.16.0</center>
                            </body>
                            </html>
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            <!-- a padding to disable MSIE and Chrome friendly error page -->
                            ));
            return 0;
    });
   // INFO(g_logger, "serverlet start");

    server->start();
}

int main(int argc, char** argv) {
    HSQ::IOManager iom(1, false, "main");
    worker.reset(new HSQ::IOManager(3, false, "worker"));
    iom.Schedule(run);
    return 0;
}
