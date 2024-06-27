
#include "db.h"

namespace HSQ
{
std::unordered_map<int ,std::function<void(sqlite3_stmt*, int index, SqlDb::JsonBuilder&)>> SqlDb::m_builderMap 
{
    {std::make_pair(SQLITE_INTEGER, [](sqlite3_stmt* stmt, int index, SqlDb::JsonBuilder& builder) {
        builder.Int64(sqlite3_column_int64(stmt, index));
    }) },

    {std::make_pair(SQLITE_FLOAT, [](sqlite3_stmt* stmt, int index, SqlDb::JsonBuilder& builder) {
        builder.Double(sqlite3_column_double(stmt, index));
    }) },

    {std::make_pair(SQLITE_TEXT, [](sqlite3_stmt* stmt, int index, SqlDb::JsonBuilder& builder) {
        builder.String((const char*)sqlite3_column_text(stmt, index));
    }) },

    {std::make_pair(SQLITE_NULL, [](sqlite3_stmt* stmt, int index, SqlDb::JsonBuilder& builder) {
        builder.Null();
    }) }

};


std::unordered_map<int, std::function<SqlDb::SqliteValue(sqlite3_stmt*, int)>> SqlDb::m_valmap = 
{
    { std::make_pair(SQLITE_INTEGER, [](sqlite3_stmt* stmt, int index) { 
        return sqlite3_column_int64(stmt, index); })},
    
    { std::make_pair(SQLITE_FLOAT, [](sqlite3_stmt* stmt, int index) { 
        return sqlite3_column_double(stmt, index); })},
    
    //{ std::make_pair(SQLITE_BLOB, [](sqlite3_stmt* stmt, int index) { 
    //    return std::string((const char*)sqlite3_column_blob(stmt, index)); })},
    
    { std::make_pair(SQLITE_TEXT, [](sqlite3_stmt* stmt, int index) { 
        return std::string((const char*)sqlite3_column_text(stmt, index)); })},

    { std::make_pair(SQLITE_INTEGER, [](sqlite3_stmt* stmt, int index) { 
        return nullptr; })}
};

void SqlDb::BuildJsonValue(sqlite3_stmt* stmt, int index) {
    int type = sqlite3_column_type(stmt, index);

    auto it = m_builderMap.find(type);
    if(it == m_builderMap.end()) {
        throw std::logic_error("no this type");
    }
    it->second(stmt, index, m_jsonBuilder);
}



}
