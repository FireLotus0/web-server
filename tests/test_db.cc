#include "db.h"
#include <iostream>

void test1()
{
    HSQ::SqlDb db;
    db.openDb("test.db");
    std::string sqlCreate = "CREATE TABLE if not exists TestTable(ID INTEGER NOT NULL, CODE INTEGER, V1 INTEGER, V2 INTEGER, V3 REAL, V4 TEXT);";
    
    if(!db.execute(sqlCreate)) {
       std::cout << "create fail" << std::endl;
       return;
    }
    HSQ::JsonCpp jcp;
    jcp.StartArray();
    for(int i = 0; i < 100000; i++) {
      jcp.StartObject();
      jcp.WriteJson(KEY(ID), i);
      jcp.WriteJson(KEY(CODE), i);
      jcp.WriteJson(KEY(V1), i);
      jcp.WriteJson(KEY(V2), i);
      jcp.WriteJson(KEY(V3), i + 10.25);
      jcp.WriteJson(KEY(V4), "hello world");
      jcp.EndObject();
      }
   
   jcp.EndArray();

   //插入数据库
   std::string insertSql = "INSERT INTO TestTable(ID, CODE, V1, V2, V3, V4)  VALUES(?, ?, ?, ?, ?, ?);";
   if(!db.executeJson(insertSql, jcp.GetString())) {
      std::cout << "insert fail" << std::endl;
   }

   //查询
   
   auto p = db.query("SELECT * from TestTable");
   if(p == nullptr) {
      std::cout << "query fail" << std::endl;
      return;
   }
   rapidjson::Document& doc = *p;
   rapidjson::StringBuffer buffer;
   rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
   doc.Accept(writer);
   
   const std::string jsonString = buffer.GetString();
   std::cout << jsonString;
}

void test2()
{

}

void test3()
{

}

int main() 
{
   test1();
   return 0;
}