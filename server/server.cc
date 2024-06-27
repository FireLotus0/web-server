#include "httpserver.h"
#include "macro.h"
#include "db.h"
#include "dao.h"
#include <iostream>

static HSQ::Logger::ptr g_logger = GET_LOGGER("server");

#define TOSTRING(...) #__VA_ARGS__


HSQ::IOManager::ptr worker;
void run() {
 
    HSQ::http::HttpServer::ptr server(new HSQ::http::HttpServer(true, worker.get(), HSQ::IOManager::GetThis()));
    HSQ::Address::ptr addr = HSQ::IPv4Address::Create("0.0.0.0", 8020);
    while(!server->bind(addr)) {
       sleep(2);
    }
    auto sd = server->getServletDispatch();
    // 门诊记录
    sd->addServlet("/HSQ/medical", HSQ::DAO::GetInstance()->getMedicalQuery());

    //登录校验 
    sd->addGlobServlet("/HSQ/login", HSQ::DAO::GetInstance()->getLogin());

    //修改电子病历
    sd->addGlobServlet("/HSQ/modifyMedical", HSQ::DAO::GetInstance()->getMedicalModify());

    //提交检查项目
    sd->addGlobServlet("/HSQ/commitCheck", HSQ::DAO::GetInstance()->getCheckProject());

    // 数据中心使用
    sd->addServlet("/HSQ/patient_info", [](HSQ::http::HttpRequest::ptr req
                ,HSQ::http::HttpResponse::ptr rsp
                ,HSQ::http::HttpSession::ptr session) {
            
            HSQ::SqlDb db;
            db.openDb("hospital.db");
            auto p = db.query("SELECT  s_patient_name, s_age, s_patient_id, s_first_diagnose, \
                         s_patient_sex, s_phone, s_addr, s_date from t_medical_record");
            rapidjson::Document& doc = *p;
            rapidjson::StringBuffer buffer;
            rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
            doc.Accept(writer);
            const std::string jsonString = buffer.GetString();
            rsp->setHeader("Content-Type", "application/json");
            rsp->setBody(jsonString);
            return 0;
    });
  
    // 药品查询
    sd->addServlet("/HSQ/medicine", HSQ::DAO::GetInstance()->getMedicineQuery());   
    
    //处方提交
    sd->addServlet("/HSQ/prescription", HSQ::DAO::GetInstance()->getPrescription());   

    // 住院通知单
    sd->addServlet("/HSQ/notify", HSQ::DAO::GetInstance()->getNotify());
    
    // 诊断证明
    sd->addServlet("/HSQ/prove", HSQ::DAO::GetInstance()->getProve());    

    sd->addGlobServlet("/HSQx/*", [](HSQ::http::HttpRequest::ptr req
                ,HSQ::http::HttpResponse::ptr rsp
                ,HSQ::http::HttpSession::ptr session) {
            rsp->setBody(TOSTRING(<html>
                            <head><title>404 Not Found</title></head>
                            <body>
                            <center><h1>404 Not Found</h1></center>
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
    server->start();
}

int main(int argc, char** argv) {
    HSQ::IOManager iom(1, false, "main");
    worker.reset(new HSQ::IOManager(3, false, "worker"));
    iom.Schedule(run);
    return 0;
}