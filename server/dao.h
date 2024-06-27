
#pragma once

#include "http.h"
#include "httpsession.h"
#include "util.h"
#include "jsoncpp.h"
#include "db.h"

#include <mutex>
#include <functional>

namespace HSQ
{

//数据库访问逻辑处理
class Dao
{
    
public:
    using FUNCOBJ = std::function<int32_t(HSQ::http::HttpRequest::ptr, HSQ::http::HttpResponse::ptr, HSQ::http::HttpSession::ptr)>; 
    //获取门诊记录
    int32_t login(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //查询电子病历
    int32_t queryMedicalRecord(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //修改电子病历
    int32_t modifyMedicalRecord(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //提交检查项目
    int32_t commitCheckProject(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //查询药品记录
    int32_t queryMedicine(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //提交处方
    int32_t commitPrescription(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //提交住院通知书
    int32_t commitNotify(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);

    //提交诊断证明书
    int32_t commitProve(HSQ::http::HttpRequest::ptr request, HSQ::http::HttpResponse::ptr response, HSQ::http::HttpSession::ptr session);


    FUNCOBJ getLogin();

    FUNCOBJ getMedicalQuery();

    FUNCOBJ getMedicalModify();

    FUNCOBJ getCheckProject();

    FUNCOBJ getMedicineQuery();

    FUNCOBJ getNotify();

    FUNCOBJ getProve();

    FUNCOBJ getPrescription();


private:
    std::string getResult(bool res);
};

using DAO = HSQ::Singleton<Dao>;
}
