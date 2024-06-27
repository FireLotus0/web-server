#include "dao.h"

#define HAS_PARAM(document, Parm, Type) \
if(document.HasMember(#Parm)) { \
    Parm = document[#Parm].Get##Type(); \
}

#define STR(Parm) std::string(" 'Parm' ")

#define STD_STRING(Parm) std::string(##Parm)

namespace HSQ  
{

int32_t Dao::login(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) 
{
    rapidjson::Document document;
    HSQ::SqlDb db;
    db.openDb("hospital.db");

     // 解析字符串为 JSON 对象
    document.Parse(req->getBody().c_str());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    std::string name = document["user"].GetString();
    std::string passwd = document["passwd"].GetString();
    const std::string sql = "select * from t_doctor  where  s_name = '" + name + "' and s_passwd = '" + passwd + "'";
    auto p = db.query(sql);
    rapidjson::Document& doc = *p;
    rapidjson::StringBuffer resBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> resWriter(resBuffer);
    doc.Accept(resWriter);
    const std::string jsonString = resBuffer.GetString();
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(jsonString);
    return 0;
}

//查询电子病历
int32_t Dao::queryMedicalRecord(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) 
{
    rapidjson::Document document;
    HSQ::SqlDb db;
    db.openDb("hospital.db");

    std::cout << req->getBody();
     // 解析字符串为 JSON 对象
    document.Parse(req->getBody().c_str());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    std::string s_patient_name, s_patient_sex, s_doctor_name, s_make_date, s_patient_id, s_insurance_id;
    std::string sql;
    int s_type = -1;

    if(!document.HasParseError()) {
        HAS_PARAM(document, s_patient_name, String)
        HAS_PARAM(document, s_patient_sex, String)
        HAS_PARAM(document, s_doctor_name, String)
        HAS_PARAM(document, s_make_date, String)
        HAS_PARAM(document, s_patient_id, String)
        HAS_PARAM(document, s_insurance_id, String)
        HAS_PARAM(document, s_type, Int)
    }

    if(s_patient_name.empty() && s_patient_sex.empty() && s_doctor_name.empty() && s_make_date.empty() 
        && s_patient_id.empty() && s_insurance_id.empty() && s_type == -1) {
        sql = "select * from t_medical_record";
    } else {
        bool useJoin = false;
        sql = "select * from t_medical_record where ";
        if(!s_patient_name.empty()) { sql += " s_patient_name like '%" + s_patient_name + "' "; useJoin = true; } 
        if(!s_patient_sex.empty()) {
            sql += useJoin ? " and s_patient_sex = '" + s_patient_sex + "'" : " s_patient_sex = '" + s_patient_sex + "'";
            if(!useJoin) { useJoin = true; }
        }
        if(!s_doctor_name.empty()) {
            sql += useJoin ? " and s_doctor_name like '%" + s_doctor_name + "%' " : 
                              " s_doctor_name like '%" + s_doctor_name + "%' ";
            if(!useJoin) { useJoin = true; }
        }
        if(!s_make_date.empty()) {
            sql += useJoin ? " and s_make_date like '" + s_make_date + "%' " : 
                              " s_make_date like '" + s_make_date + "%' ";
            if(!useJoin) { useJoin = true; }
        }
        if(!s_patient_id.empty()) {
            sql += useJoin ? " and s_patient_id like '" + s_patient_id + "%' " : 
                              " s_patient_id like '" + s_patient_id + "%' ";
            if(!useJoin) { useJoin = true; }
        }
        if(!s_insurance_id.empty()) {
            sql += useJoin ? " and s_insurance_id like '" + s_insurance_id + "%' " : 
                              " s_insurance_id like '" + s_insurance_id + "%' ";
            if(!useJoin) { useJoin = true; }
        }
        if(s_type != -1) {
            sql += useJoin ? " and s_type = " + std::to_string(s_type) : 
                              " s_type = " + std::to_string(s_type);
            if(!useJoin) { useJoin = true; }
        }
    }
    
    std::cout << "电子病历查询语句为：" << sql << std::endl;
    auto p = db.query(sql);
    rapidjson::Document& doc = *p;
    rapidjson::StringBuffer resBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> resWriter(resBuffer);
    doc.Accept(resWriter);
    const std::string jsonString = resBuffer.GetString();
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(jsonString);
    return 0;
}
 
//插入电子病历
int32_t Dao::modifyMedicalRecord(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) {
    rapidjson::Document document;
    HSQ::SqlDb db;
    db.openDb("hospital.db");

     // 解析字符串为 JSON 对象
    document.Parse(req->getBody().c_str());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);
    std::string operation;
    if(!document.HasParseError()) {
        HAS_PARAM(document, operation, String)
    }
    if(operation.empty()) {
        rsp->setHeader("Content-Type", "application/json");
        rsp->setBody(getResult(false));
        return 0;
    }
    bool res = false;
    if(operation == "insert") {
        std::string insertMedical = "INSERT INTO t_medical_record VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, \
                                            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        res = db.executeJson(insertMedical, buffer.GetString());
    }  else if(operation == "update") {
            std::string s_patient_id;               HAS_PARAM(document, s_patient_id, String)
            std::string s_expense_source;           HAS_PARAM(document, s_expense_source, String)
            std::string s_insurance_id;             HAS_PARAM(document, s_insurance_id, String)
            std::string s_patient_name;             HAS_PARAM(document, s_patient_name, String)
            std::string s_patient_sex;              HAS_PARAM(document, s_patient_sex, String)
            std::string s_addr;                     HAS_PARAM(document, s_addr, String)
            std::string s_phone;                    HAS_PARAM(document, s_phone, String)
            std::string s_job;                      HAS_PARAM(document, s_job, String)
            std::string s_id_type;                  HAS_PARAM(document, s_id_type, String)
            std::string s_date;                     HAS_PARAM(document, s_date, String)
            std::string s_main_suit;                HAS_PARAM(document, s_main_suit, String)
            std::string s_present_illness;          HAS_PARAM(document, s_present_illness, String)
            std::string s_body_check;               HAS_PARAM(document, s_body_check, String)
            std::string s_blood_pressure;           HAS_PARAM(document, s_blood_pressure, String)
            std::string s_empty_blood_glucose;      HAS_PARAM(document, s_empty_blood_glucose, String)
            std::string s_meal_blood_glucose;       HAS_PARAM(document, s_meal_blood_glucose, String)
            std::string s_help_check;               HAS_PARAM(document, s_help_check, String)
            std::string s_first_diagnose;           HAS_PARAM(document, s_first_diagnose, String)
            std::string s_deal;                     HAS_PARAM(document, s_deal, String)
            std::string s_doctor_name;              HAS_PARAM(document, s_doctor_name, String)
            std::string s_make_date;                HAS_PARAM(document, s_make_date, String)
            std::string s_department;               HAS_PARAM(document, s_department, String)       
            int s_age;                              HAS_PARAM(document, s_age, Int)   
            int s_doctor_id;                        HAS_PARAM(document, s_doctor_id, Int)   
            int s_type;                             HAS_PARAM(document, s_type, Int)   

        std::string updateMedical = 
            std::string(" UPDATE t_medical_record SET ") +
            std::string(" s_expense_source = '") + s_expense_source + std::string("', ") +
            std::string(" s_insurance_id = '") + s_insurance_id + std::string("', ") +
            std::string(" s_patient_name = '") + s_patient_name+ std::string("', ") +
            std::string(" s_patient_sex = '") + s_patient_sex + std::string("', ") +
            std::string(" s_addr = '") + s_addr + std::string("', ") +
            std::string(" s_phone = '") + s_phone + std::string("', ") +
            std::string(" s_job = '") + s_job + std::string("', ") +
            std::string(" s_id_type = '") + s_id_type + std::string("', ") +
            std::string(" s_date = '") + s_date + std::string("', ") +
            std::string(" s_main_suit = '") + s_main_suit + std::string("', ") +
            std::string(" s_present_illness = '") + s_present_illness + std::string("', ") +
            std::string(" s_body_check = '") + s_body_check + std::string("', ") +
            std::string(" s_blood_pressure = '") + s_blood_pressure + std::string("', ") +
            std::string(" s_empty_blood_glucose = '") + s_empty_blood_glucose + std::string("', ") +
            std::string(" s_meal_blood_glucose = '") + s_meal_blood_glucose + std::string("', ") +
            std::string(" s_help_check = '") + s_help_check + std::string("', ") +
            std::string(" s_first_diagnose = '") + s_first_diagnose + std::string("', ") +
            std::string(" s_deal = '") + s_deal + std::string("', ") +
            std::string(" s_doctor_name = '") + s_doctor_name + std::string("', ") +
            std::string(" s_make_date = '") + s_make_date + std::string("', ") +
            std::string(" s_department = '") + s_department + std::string("', ") +
            std::string(" s_age = ") + std::to_string(s_age)  + std::string(", ") +
            std::string(" s_doctor_id = ") + std::to_string(s_doctor_id) + std::string(", ") +
            std::string(" s_type = ") + std::to_string(s_type) + 
            std::string(" WHERE s_patient_id = '")  + s_patient_id + std::string("'");
        std::cout << "更新电子病历：" << updateMedical << std::endl;
        res = db.execute(updateMedical);
    }

    
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(getResult(res));
    return 0;

}

//提交检查项目
int32_t Dao::commitCheckProject(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) 
{
    HSQ::SqlDb db;
    db.openDb("hospital.db");

    auto json = req->getBody();
    if(json.empty()) {
        rsp->setHeader("Content-Type", "application/json");
        rsp->setBody(getResult(false));
        return 0;
    }

    std::string sql = "INSERT INTO t_check_record VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    bool res = db.executeJson(sql, json.c_str());
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(getResult(res));
    return 0;
}

//提交处方
int32_t Dao::commitPrescription(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) {
    HSQ::SqlDb db;
    db.openDb("hospital.db");
    auto json = req->getBody();
    if(json.empty()) {
        rsp->setHeader("Content-Type", "application/json");
        rsp->setBody(getResult(false));
        return 0;
    }
    std::string sql = "INSERT INTO t_prescription_record(s_patient_id, s_patient_name, s_time, s_cost,  s_prescription, s_doctor_id, s_doctor_name) VALUES(?, ?, ?, ?, ?, ?, ?)";
    bool res = db.executeJson(sql, json.c_str());
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(getResult(res));
    return 0;
}


//提交住院通知书
int32_t Dao::commitNotify(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session)
{
    HSQ::SqlDb db;
    db.openDb("hospital.db");
    auto json = req->getBody();
    if(json.empty()) {
        rsp->setHeader("Content-Type", "application/json");
        rsp->setBody(getResult(false));
        return 0;
    }
    std::string sql = "INSERT INTO t_notify VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    bool res = db.executeJson(sql, json.c_str());
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(getResult(res));
    return 0;
}

    //提交诊断证明书
int32_t Dao::commitProve(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session){
    HSQ::SqlDb db;
    db.openDb("hospital.db");
    auto json = req->getBody();
    if(json.empty()) {
        rsp->setHeader("Content-Type", "application/json");
        rsp->setBody(getResult(false));
        return 0;
    }
    std::string sql = "INSERT INTO t_prove VALUES( ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    bool res = db.executeJson(sql, json.c_str());
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(getResult(res));
    //std::cout << "插入诊断结果" << res << std::endl;

    return 0;
}


//查询药品记录
int32_t Dao::queryMedicine(HSQ::http::HttpRequest::ptr req, HSQ::http::HttpResponse::ptr rsp, HSQ::http::HttpSession::ptr session) 
{
    rapidjson::Document document;
    HSQ::SqlDb db;
    db.openDb("hospital.db");

    std::cout << req->getBody();
     // 解析字符串为 JSON 对象
    document.Parse(req->getBody().c_str());
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    std::string s_pharmacy, s_name;
    std::string sql;
    int s_prescription_type, s_skin_test, s_medical_type;

    if(!document.HasParseError()) {
        HAS_PARAM(document, s_pharmacy, String)
        HAS_PARAM(document, s_name, String)
        HAS_PARAM(document, s_prescription_type, Int)
        HAS_PARAM(document, s_skin_test, Int)
        HAS_PARAM(document, s_medical_type, Int)
        sql = "SELECT * FROM t_medicine_record WHERE s_pharmacy = '" + s_pharmacy + "' AND " +
            "s_prescription_type = " + std::to_string(s_prescription_type) + " AND " +
            " s_skin_test = " + std::to_string(s_skin_test) + " AND s_medical_type = " + std::to_string(s_medical_type);
        if(!s_name.empty()) {
            sql += " AND s_name LIKE '%" + s_name +"%'";
        }
    } else {
        sql = "SELECT * FROM t_medicine_record";
    }

    auto p = db.query(sql);
    rapidjson::Document& doc = *p;
    rapidjson::StringBuffer resBuffer;
    rapidjson::Writer<rapidjson::StringBuffer> resWriter(resBuffer);
    doc.Accept(resWriter);
    const std::string jsonString = resBuffer.GetString();
    rsp->setHeader("Content-Type", "application/json");
    rsp->setBody(jsonString);
    std::cout << "medicine query:" << sql << " res:" << jsonString;

    return 0;
}


Dao::FUNCOBJ Dao::getLogin() {
    return std::bind(&Dao::login, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getMedicalQuery() {
    return std::bind(&Dao::queryMedicalRecord, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getMedicalModify() {
    return std::bind(&Dao::modifyMedicalRecord, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getCheckProject() {
    return std::bind(&Dao::commitCheckProject, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getMedicineQuery() {
    return std::bind(&Dao::queryMedicine, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getNotify() {
    return std::bind(&Dao::commitNotify, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getProve() {
    return std::bind(&Dao::commitProve, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

Dao::FUNCOBJ Dao::getPrescription() {
    return std::bind(&Dao::commitPrescription, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}


std::string Dao::getResult(bool res) {
    JsonCpp json;
    json.StartArray();
    json.StartObject();
    if(res) {
        json.WriteJson("result", true);
    } else {
        json.WriteJson("result", false);
    }
    json.EndObject();
    json.EndArray();
    return json.GetString();
}

}




