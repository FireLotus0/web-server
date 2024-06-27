#include "db.h"
#include "util.h"
#include <iostream>
#include "jsoncpp.h"
#include <vector>

struct Patient
{
   std::string s_patient_id;
   std::string s_expense_source;
   std::string s_insurance_id;
   int s_doctor_id;
   int s_type;
   std::string s_patient_name;
   int s_age;
   std::string s_patient_sex;
   std::string s_addr;
   std::string s_phone;
   std::string s_job;
   std::string s_id_type;
   std::string s_date;
   std::string s_main_suit;
   std::string s_present_illness;
   std::string s_body_check;
   std::string s_blood_pressure;
   std::string s_empty_blood_glucose;
   std::string s_meal_blood_glucose;
   std::string s_help_check;
   std::string s_first_diagnose;
   std::string s_deal;
   std::string s_doctor_name;
   std::string s_make_date;
   std::string s_department;
};
std::vector<Patient> patientData;
Patient patient{"340104198711052380",  // 病人ID
                "个人支付",      
                "00000000000",   
                31202110,
                0, 
                "范寻菱",
                40, 
               "女", 
               "安徽省合肥市西市区",
                "13734995687", 
                "农民", 
                "居民身份证", 
                HSQ::ElapsedTime::CurrentTimeStr(), 
               "头痛,发烧",
               "无", 
               "一般情况:患者精神状态良好,意识清醒,面色稍苍白。血压:收缩压120毫米汞柱,舒张压80毫米汞柱,脉压40毫米汞柱,脉搏规则。身高:170 cm 体重:75 kg 体质指数(BMI):25.95(超重)",
                "120/80 mmHg", 
                "5.6 mmol/L(正常范围)", 
                "7.8 mmol/L(正常范围)", 
                "血液常规检查:包括血红蛋白、白细胞、常规血清,以评估血液情况。 肝功能检查:计划进行以评估肝功能状态。",
               "感冒。", 
               "开药",
               "张飞",
                HSQ::ElapsedTime::CurrentTimeStr(),
               "外科"};

struct Medicine {
   std::string s_medicine_id;	
   std::string s_name;						
   std::string s_bar_code ;						
   std::string s_size;					
   std::string s_unit;						
   std::string s_manufacturer;				
   std::string s_approval_number;								
   std::string s_price;					
   std::string s_pharmacy;						
   std::string s_tabu;						
   std::string s_adaptation_disease;		
   std::string s_adverse_reaction;		
   std::string s_special_note;				
   std::string s_note;					   
   int s_prescription_type;		// 处方类型
   int s_skin_test;				// 皮试结果
   int s_medical_type;				// 药品类型 
};
Medicine medicine{
   "pzwh90806789",  // 药品ID
   "感冒灵颗粒",    // 药品名称 
   "126739054702845",// 药品条码
   "12袋/盒",        // 药品规格
   "太极集团",       // 单位
   "江西九江制药厂", // 生产厂商
   "axhk20240507",   // 批准文号		
   "24",             // 价格
   "长安大药房",     // 药房
   "不可与甘草片同时服用", // 禁忌 
   "适应普通感冒，流感",   // 适应症
   "无",                // 不良反应
   "孕妇慎用",          // 特殊说明 
   "若服用周期超过5天，不见好转，请及时就医",      // 备注
   0,
   0,
   0
};
void test1()
{
    HSQ::SqlDb db;
    db.openDb("hospital.db");

    std::string sqlCreate = "CREATE TABLE if not exists t_medical_record("
                            "s_patient_id TEXT PRIMARY KEY NOT NULL, "  	// 病人ID
                            "s_expense_source TEXT, "			// 费用来源
                            "s_insurance_id TEXT, "				// 医保应用号
                            "s_doctor_id INTEGER,"			    // 制单医生ID
                            "s_type  INTEGER, "				// 门诊类型:0--综合 1--急诊  2--普通 3--保健
                            "s_patient_name TEXT, "				// 病人姓名
                            "s_age INTEGER, "					// 年龄
                            "s_patient_sex TEXT, "				// 病人性别
                            "s_addr TEXT, "						// 住址
                            "s_phone TEXT, "					// 电话
                            "s_job TEXT, "						// 职业类型
                            "s_id_type TEXT,"					// 证件类型
                            "s_date TEXT,"						// 发病日期
                            "s_main_suit TEXT, "				// 主诉
                            "s_present_illness TEXT,"			// 现病史
                            "s_body_check TEXT, "				// 体格检查
                            "s_blood_pressure  TEXT,"			// 血压
                            "s_empty_blood_glucose TEXT, "	// 空腹血糖
                            "s_meal_blood_glucose TEXT,"		// 餐后血糖
                            "s_help_check TEXT, "				// 辅助检查
                            "s_first_diagnose TEXT,"			// 初步诊断
                            "s_deal TEXT, "						// 处理
                            "s_doctor_name TEXT, "          // 制单医生姓名
                            "s_make_date TEXT,"					// 制单时间
                            "s_department TEXT);";				// 科室
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建病历表失败" << std::endl;
       return;
    }

    sqlCreate = "CREATE TABLE if not exists t_check_record("
                            "s_project_id TEXT ,"		      // 项目编码
                            "s_patient_id TEXT, "           // 病人ID
                            "s_note TEXT, "                 // 备注
                            "s_doctor_name TEXT, "          // 医师
                            "s_department TEXT, "				// 科室
                            "s_check_part TEXT, "				// 检查部位
                            "s_goal TEXT, "						// 检查目的
                            "s_project_name TEXT, "			// 项目名称
                            "s_project_name_f TEXT,"			// 收费项目名称
                            "s_project_size TEXT,"				// 项目规格
                            "s_price REAL, "					   // 价格
                            "s_num INTEGER,"					   // 数量
                            "s_total REAL);";					// 金额
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建检查单表失败" << std::endl;
       return;
    }

    sqlCreate = "CREATE TABLE if not exists t_medicine_record("
                            "s_medicine_id TEXT,"	// 药品ID
                            "s_name TEXT, "							// 药品名称
                            "s_bar_code TEXT, "						// 药品条码
                            "s_size TEXT, "							// 药品规格
                            "s_unit TEXT, "							// 单位
                            "s_manufacturer TEXT, "				// 生产厂商
                            "s_approval_number TEXT,"				// 批准文号						
                            "s_price REAL,"							// 价格
                            "s_pharmacy TEXT, "						// 药房
                            "s_prescription_type INTEGER, "		// 处方类型
                            "s_skin_test INTEGER, "				// 皮试结果
                            "s_medical_type INTEGER,"				// 药品类型 
                            "s_tabu TEXT, "							// 禁忌 
                            "s_adaptation_disease TEXT, "		// 适应症
                            "s_adverse_reaction TEXT, "			// 不良反应
                            "s_special_note TEXT,"					// 特殊说明
                            "s_note TEXT);";						   // 备注
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建药品表失败" << std::endl;
       return;
    }

    
   //处方表
   sqlCreate = "CREATE TABLE if not exists t_prescription_record("
                            "s_patient_id TEXT,"	            // 病人ID
                            "s_patient_name TEXT, "			   // 病人姓名
                            "s_time TEXT, "						   // 开处方时间
                            "s_cost  REAL, "							// 金额
                            "s_prescription TEXT,"					// 处方
                            "s_doctor_id TEXT, "				   // 医师ID
                            "s_doctor_name TEXT"				   // 医师姓名					
                           ");";	
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建处方表失败" << std::endl;
       return;
    }

    sqlCreate = "CREATE TABLE if not exists t_notify("
                            "s_patient_id TEXT, "// 病人ID
                            "s_patient_name TEXT, "                  // 病人姓名
                            "s_patient_sex TEXT, "                   // 性别
                            "s_patient_addr TEXT, "                  // 地址
                            "s_work_unit TEXT, "                     // 工作单位
                            "s_work_phone TEXT,"                     // 工作电话
                            "s_home_addr TEXT,"                      // 家庭住址
                            "s_home_phone TEXT, "                    // 家庭电话
                            "s_diagnose TEXT, "                      // 门诊诊断
                            "s_family_name TEXT, "                   // 联系人姓名
                            "s_family_relation TEXT,"                // 联系人关系
                            "s_family_unit TEXT, "                   // 联系人单位                      
                            "s_family_phone TEXT, "               // 联系人电话
                            "s_family_addr TEXT, "                   // 联系人住址
                            "s_fee REAL,"                            // 预交金
                            "s_date TEXT, "                          // 开单时间
                            "s_doctor TEXT, "                        // 开单医师
                            "s_department TEXT, "                    // 科室
                            "s_in_department TEXT);";                // 住院科室
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建住院通知单表失败" << std::endl;
       return;
    }

    sqlCreate = "CREATE TABLE if not exists t_prove("
                            "s_patient_id TEXT , "// 病人ID
                            "s_patient_name TEXT, "      // 病人姓名
                            "s_patient_sex  TEXT, "      // 病人性别
                            "s_age INTEGER, "            // 病人年龄
                            "s_department TEXT, "        // 科室
                            "s_first_diagnose TEXT,"     // 初步诊断
                            "s_adviser TEXT,"            // 建议
                            "s_last_modify_name TEXT,"   // 最后一次修改人
                            "s_last_modify_date TEXT, "  // 最后一次修改时间
                            "s_doctor_name TEXT);";      // 医师姓名
    
    if(!db.execute(sqlCreate)) {
       std::cout << "创建诊断证明表失败" << std::endl;
       return;
    }

      sqlCreate = "CREATE TABLE if not exists t_doctor("
                           "s_id INTEGER  PRIMARY KEY NOT NULL,"     //
                           "s_name TEXT, "				// 医生姓名
                           "s_passwd TEXT, "				// 医生密码
                           "s_department TEXT"             // 科室
                           ")";				
   
   if(!db.execute(sqlCreate)) {
      std::cout << "创建医生表失败" << std::endl;
      return;
   }
}

void testInsert() {
      HSQ::SqlDb db;
      db.openDb("hospital.db");

      //插入医生数据
      HSQ::JsonCpp jsonCpp;
      std::vector<std::string> names{"张飞", "刘备", "关羽"};
      jsonCpp.StartArray();
      int id = 100;
      for(auto name : names) {
         jsonCpp.StartObject();
         jsonCpp.WriteJson("s_id", 31202009 + id++);
         jsonCpp.WriteJson("s_name", name);
         jsonCpp.WriteJson("s_passwd", "admin");
         jsonCpp.WriteJson("s_department", "外科");
         jsonCpp.EndObject();
      }
      jsonCpp.EndArray(); 
      std::string insertDoc = "INSERT INTO t_doctor(s_id, s_name, s_passwd, s_department) VALUES(?, ?, ?, ?)";
      db.executeJson(insertDoc, jsonCpp.GetString());


      //插入门诊测试数据
      HSQ::JsonCpp jsonCppMedical;
      patientData.push_back(patient);
      jsonCppMedical.StartArray();
      for(auto data : patientData) {
         jsonCppMedical.StartObject();
         jsonCppMedical.WriteJson("s_patient_id", data.s_patient_id );  	// 病人ID
         jsonCppMedical.WriteJson("s_expense_source", data.s_expense_source );			// 费用来源
         jsonCppMedical.WriteJson("s_insurance_id ", data.s_insurance_id );				// 医保应用号
         jsonCppMedical.WriteJson("s_doctor_id,", data.s_doctor_id );			    // 制单医生ID
         jsonCppMedical.WriteJson("s_type", data.s_type );				// 门诊类型:0--综合 1--急诊  2--普通 3--保健
         jsonCppMedical.WriteJson("s_patient_name", data.s_patient_name );				// 病人姓名
         jsonCppMedical.WriteJson("s_age", data.s_age );					// 年龄
         jsonCppMedical.WriteJson("s_patient_sex", data.s_patient_sex );				// 病人性别
         jsonCppMedical.WriteJson("s_addr", data.s_addr );						// 住址
         jsonCppMedical.WriteJson("s_phone", data.s_phone );					// 电话
         jsonCppMedical.WriteJson("s_job", data.s_job );						// 职业类型
         jsonCppMedical.WriteJson("s_id_type", data.s_id_type );					// 证件类型
         jsonCppMedical.WriteJson("s_date", data.s_date );						// 发病日期
         jsonCppMedical.WriteJson("s_main_suit", data.s_main_suit );				// 主诉
         jsonCppMedical.WriteJson("s_present_illness", data.s_present_illness );			// 现病史
         jsonCppMedical.WriteJson("s_body_check", data.s_body_check );				// 体格检查
         jsonCppMedical.WriteJson("s_blood_pressure", data.s_blood_pressure );			// 血压
         jsonCppMedical.WriteJson("s_empty_blood_glucose", data.s_empty_blood_glucose );	// 空腹血糖
         jsonCppMedical.WriteJson("s_meal_blood_glucose", data.s_meal_blood_glucose );		// 餐后血糖
         jsonCppMedical.WriteJson("s_help_check", data.s_help_check );				// 辅助检查
         jsonCppMedical.WriteJson("s_first_diagnose", data.s_first_diagnose );			// 初步诊断
         jsonCppMedical.WriteJson("s_deal", data.s_deal );						// 处理
         jsonCppMedical.WriteJson("s_doctor_name", data.s_doctor_name);						// 处理
         jsonCppMedical.WriteJson("s_make_date", data.s_make_date );					// 制单时间
         jsonCppMedical.WriteJson("s_department", data.s_department );			// 科室
         jsonCppMedical.EndObject();
      }
      jsonCppMedical.EndArray();
      std::string insertMedical = "INSERT INTO t_medical_record VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
      db.executeJson(insertMedical, jsonCppMedical.GetString());


      //插入药品测试数据
      HSQ::JsonCpp jsonCppMedicine;
      jsonCppMedicine.StartArray();
      jsonCppMedicine.StartObject();
      jsonCppMedicine.WriteJson("s_medicine_id", medicine.s_medicine_id);
      jsonCppMedicine.WriteJson("s_name", medicine.s_name);
      jsonCppMedicine.WriteJson("s_bar_code", medicine.s_bar_code);
      jsonCppMedicine.WriteJson("s_size", medicine.s_size);
      jsonCppMedicine.WriteJson("s_unit", medicine.s_unit);
      jsonCppMedicine.WriteJson("s_manufacturer", medicine.s_manufacturer);
      jsonCppMedicine.WriteJson("s_approval_number", medicine.s_approval_number);
      jsonCppMedicine.WriteJson("s_price", medicine.s_price);
      jsonCppMedicine.WriteJson("s_pharmacy", medicine.s_pharmacy);
      jsonCppMedicine.WriteJson("s_prescription_type", medicine.s_prescription_type);
      jsonCppMedicine.WriteJson("s_skin_test", medicine.s_skin_test);
      jsonCppMedicine.WriteJson("s_medical_type", medicine.s_medical_type);
      jsonCppMedicine.WriteJson("s_tabu", medicine.s_tabu);
      jsonCppMedicine.WriteJson("s_adverse_reaction", medicine.s_adverse_reaction);
      jsonCppMedicine.WriteJson("s_special_note", medicine.s_special_note);
      jsonCppMedicine.WriteJson("s_note", medicine.s_note);
      jsonCppMedicine.EndObject();
      jsonCppMedicine.EndArray(); 
      std::string insertMedicine = "INSERT INTO t_medicine_record VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
      db.executeJson(insertMedicine, jsonCppMedicine.GetString());
}

void testInsertPrescription() {
   //SQ::SqlDb db;
   //db.openDb("hospital.db");
   //
   //SQ::JsonCpp jsonPrescription;
   //sonPrescription.StartObject();
   //sonCppMedicine.WriteJson("s_patient_id", "");

   //sonPrescription.EndObject();


   //std::cout << json.c_str() << std::endl;
   //std::string sql = "INSERT INTO t_prescription_record(s_patient_id, s_time,  \
   //                s_patient_name, s_cost, , s_prescription,s_doctor_id, s_doctor_name) VALUES(?, ?, ?, ?, ?, ?, ?)";
   //bool res = db.executeJson(sql, json.c_str());
   //rsp->setHeader("Content-Type", "application/json");
   //rsp->setBody(getResult(res));
}

int main() 
{
   test1();
   testInsert();
   //testJson();
   return 0;
}
