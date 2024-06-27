#pragma once

#include <sqlite3.h>
#include <string>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cctype>

#include "variant.h"
#include "jsoncpp.h"

#define KEY(name) std::string(#name)

namespace HSQ
{

template<int...>
struct IndexTuple{};

template<int N, int...Indexes>
struct MakeIndexes : MakeIndexes<N - 1, N -1, Indexes...> {};

template<int...indexes>
struct MakeIndexes<0, indexes...> 
{
    typedef IndexTuple<indexes...> type;
};

class SqlDb 
{


public:
    using ptr = std::shared_ptr<SqlDb>;
    using JsonBuilder = rapidjson::Writer<rapidjson::StringBuffer>;
    typedef HSQ::Variant<double, int, uint32_t, sqlite3_int64, char*, const char*, /*BLOB,*/ std::string, nullptr_t> SqliteValue;

private:
    std::string m_name;
    sqlite3* m_handle{nullptr};
    sqlite3_stmt* m_statement{nullptr};
    int m_code;
    rapidjson::StringBuffer m_buf;
    rapidjson::Writer<rapidjson::StringBuffer> m_jsonBuilder;
    static std::unordered_map<int, std::function<SqliteValue(sqlite3_stmt*, int)>> m_valmap;
    
    static std::unordered_map<int ,std::function<void(sqlite3_stmt*, int index, JsonBuilder&)>> m_builderMap;
public:

    SqlDb() : m_jsonBuilder(m_buf) {}

    explicit SqlDb(const std::string& name)  : m_jsonBuilder(m_buf) {
        m_name = name;
        openDb(name);
    }

    ~SqlDb() { closeDb(); }

    bool openDb(const std::string& db_name) {
        m_code = sqlite3_open(db_name.data(), &m_handle);
        return (m_code == SQLITE_OK);
    }

    bool closeDb() {
         if(m_handle == nullptr) {
        return true;
        }
        sqlite3_finalize(m_statement);
        m_code = closeDbHandle();
        m_statement = nullptr;
        m_handle = nullptr;
        return (m_code == SQLITE_OK);
    }

    bool begin() { return execute("BEGIN"); }

    bool rollback() { return execute("ROLLBACK"); }

    bool commit() { return execute("COMMIT"); }

    int getLastErrorCode() const {
        return m_code;
    }

    template<typename Tuple>
    bool executeTuple(const std::string& sqlStr, Tuple&& t) {
        if(!prepare(sqlStr)) {
            return false;
        }

        return executeTuple(MakeIndexes<std::tuple_size<Tuple>::value>::type(), 
                            std::forward<Tuple>(t));
    }


    template<int...Indexes, class Tuple>
    bool executeTuple(IndexTuple<Indexes...>&& in, Tuple&& t) {
        if(SQLITE_OK != bindParams(m_statement, 1, std::get<Indexes>(std::forward<Tuple>(t))...)) {
            return false;
        }

        m_code = sqlite3_step(m_statement);
        sqlite3_reset(m_statement);
        return m_code == SQLITE_DONE;
    }

    bool executeJson(const std::string& sqlStr, const char* json) {
        rapidjson::Document doc;
        doc.Parse<0>(json);
        if(doc.HasParseError()) {
            std::cout << "解析json错误：" << doc.GetParseError() << std::endl; 
            return false;
        }
        if(!prepare(sqlStr)) {
            std::cout << "解析sql错误:" <<sqlStr<< std::endl;
            return false;
        }
        return JsonTransaction(doc);
    }

    bool executeJson(const rapidjson::Value& val) {
        auto p = val.GetKeyPtr();
        for(size_t i =0, size = val.GetSize(); i < size; i++) {
            const char* key = val.GetKey(p++);
            auto& t = val[key];
            bindJsonValue(t, i + 1);
        }

        m_code = sqlite3_step(m_statement);
        sqlite3_reset(m_statement);
        return SQLITE_DONE == m_code;
    }

    void bindJsonValue(const rapidjson::Value& t, int index) {
        auto type = t.GetType();

        if(type == rapidjson::kNullType) {
            m_code = sqlite3_bind_null(m_statement, index);
        } else if(type == rapidjson::kStringType) {
            m_code == sqlite3_bind_text(m_statement, index, t.GetString(), -1, SQLITE_STATIC) ;
        } else if(type == rapidjson::kNumberType) {
            bindNumber(t, index);
        } else {
            throw std::invalid_argument("no this type");
        }
    }

    void bindNumber(const rapidjson::Value& t, int index) {
        if(t.IsInt() || t.IsUint()) {
            m_code == sqlite3_bind_int(m_statement, index, t.GetInt()); 
        } else if(t.IsInt64() || t.IsUint64()) {
            m_code = sqlite3_bind_int64(m_statement, index, t.GetInt64());
        } else {
            m_code = sqlite3_bind_double(m_statement, index, t.GetDouble());
        }
    }

    int bindParams(sqlite3_stmt* statement, int current) {
        return SQLITE_OK;
    }

    /**
    * @brief 不带占位符，执行SQL，不带返回结果，如insert,update,delete
    */
    bool execute(const std::string& sqlStr)
    {
        m_code = sqlite3_exec(m_handle, sqlStr.data(), nullptr, nullptr, nullptr);
        return m_code == SQLITE_OK;
    }

    /**
    * @brief 带占位符，执行SQL，不带返回结果，如insert,update,delete
    */
    template<typename...Args>
    bool execute(const std::string& sqlStr, Args&&...args) 
    {
        if(!prepare(sqlStr)) {
            return false;
        }
        return executeArgs(std::forward<Args>(args)...);
    }

    /**
    * @brief 执行汇聚函数，返回一个值，
    */
    template<typename R = sqlite_int64, typename...Args>
    R executeR(const std::string& sqlStr, Args&&...args) 
    {
        if(!prepare(sqlStr)) {
            return getErrorVal<R>();
        }
        //绑定sql脚本中的参数
        if(SQLITE_OK != bindParams(m_statement, 1, std::forward<Args>(args)...)) {
            return getErrorVal<R>();
        }

        m_code = sqlite3_step(m_statement);

        if(m_code != SQLITE_ROW) {
            return getErrorVal<R>();
        }

        SqliteValue val = getValue(m_statement, 0);
        R res = val.get<R>();
        sqlite3_reset(m_statement);
        return res;
    }

    //取列的值
    SqliteValue getValue(sqlite3_stmt* stmt, const int& index)
    {
        int type = sqlite3_column_type(stmt, index);
        //根据列的类型取值
        auto it = m_valmap.find(type);
        if(it == m_valmap.end()) {
            throw std::logic_error("can not find type");
        }
        return it->second(stmt, index);
    }

    template<typename T, typename...Args>
    int bindParams(sqlite3_stmt* statement, int current, T&& first, Args&&...args) {
        bindValue(statement, current, first);
        if(m_code != SQLITE_OK) {
            return m_code;
        }

        bindParams(statement, current + 1, std::forward<Args>(args)...);
        return m_code;
    }

    template<typename...Args>
    std::shared_ptr<rapidjson::Document> query(const std::string& key, Args...args) {
        if(!prepare(key)) { //, std::forward<Args>(args)...)
            std::cout << "解析sql失败 in query()" << std::endl;
            return nullptr;
        }

        auto doc  = std::make_shared<rapidjson::Document>();
        m_buf.Clear();

        BuildJsonObject();

        doc->Parse<0>(m_buf.GetString());
        return doc;
    }

    void BuildJsonObject() {
        int colCount = sqlite3_column_count(m_statement);

        m_jsonBuilder.StartArray();

        while(true) {
            m_code = sqlite3_step(m_statement);
            if(m_code == SQLITE_DONE) {
                break;
            }
            BuildJsonArray(colCount);
        }
        m_jsonBuilder.EndArray();
        sqlite3_reset(m_statement);
    }

    void toUpper(char* str) {
        std::size_t length = std::strlen(str);
        for (std::size_t i = 0; i < length; ++i) {
            str[i] = std::toupper(static_cast<unsigned char>(str[i]));
        }
    }

    void BuildJsonArray(int colCount) {
        m_jsonBuilder.StartObject();
        for(int i = 0; i < colCount; i++) {
            char* name = (char*)sqlite3_column_name(m_statement, i);
            //toUpper(name);
            //m_jsonBuilder.String(name);
            m_jsonBuilder.Key(name);
            BuildJsonValue(m_statement, i);
        }
        m_jsonBuilder.EndObject();
    }


    void BuildJsonValue(sqlite3_stmt* stmt, int index);

private:
    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value>::type bindValue(sqlite3_stmt* statement, int current, T t)
    {
        m_code = sqlite3_bind_double(statement, current, std::forward<T>(t));
    } 

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value>::type bindValue(sqlite3_stmt* statement, int current, T t)
    {
        bindIntValue(statement, current, t);
    }

    template<typename T>
    typename std::enable_if<std::is_same<T, int64_t>::value || std::is_same<T, uint64_t>::value>::type
        bindIntValue(sqlite3_stmt* statement, int current, T t)
    {
        m_code = sqlite3_bind_int64(statement, current, std::forward<T>(t));
    } 

    template<typename T>
    typename std::enable_if<!std::is_same<int64_t, T>::value || !std::is_same<uint64_t, T>::value>::type
        bindIntValue(sqlite3_stmt* statement, int current, T t)
    {
        m_code = sqlite3_bind_int(statement, current, std::forward<T>(t));
    }

    template<typename T>
    typename std::enable_if<std::is_same<std::string, T>::value>::type bindValue(sqlite3_stmt* statement, int current, const T& t) 
    {
        m_code = sqlite3_bind_text(statement, current, t.data()
                    , t.length(), SQLITE_TRANSIENT);
    }

    template<typename T>
    typename std::enable_if<std::is_same<char*, T>::value || std::is_same<const char*, T>::value>::type
        bindValue(sqlite3_stmt* statement, int current, T t)
    {
        m_code = sqlite3_bind_text(statement, current, t, strlen(t) + 1, 
                                    SQLITE_TRANSIENT);
    }

    //template<typename T>
    //typename std::enable_if<std::is_same<BLOB, T>::value>::type bindValue(sqlite3_stmt* statement, int current, const T& t)
    //{
    //    m_code = sqlite3_bind_blob(statement, current, t.pBuf， t.size, 
    //                                SQLITE_TRANSIENT);
    //}

    template<typename T>
    typename std::enable_if<std::is_same<nullptr_t, T>::value>::type bindValue(sqlite3_stmt* statement, int current, const T& t)
    {
        m_code = sqlite3_bind_null(statement, current);
    }

private:
    bool prepare(const std::string& sqlStr) {
        m_code = sqlite3_prepare_v2(m_handle, sqlStr.data(), -1, &m_statement, nullptr);

        if(m_code != SQLITE_OK) {
            return false;
        }
        return true;
    }
    
    template<typename...Args>
    bool executeArgs(Args&&...args) 
    {
        if(SQLITE_OK != bindParams(m_statement, 1, std::forward<Args>(args)...)) {
            return false;
        }

        m_code = sqlite3_step(m_statement);
        sqlite3_reset(m_statement);
        return m_code == SQLITE_DONE;
    }

private:
    template<typename T>
    typename std::enable_if<std::is_arithmetic<T>::value>::type getErrorVal() {
        return T(-6666);
    }

    //template<typename T>
    //typename std::enable_if<!std::is_arithmetic<T>::value && std::is_same<T, BLOB>::value>::type getErrorVal() {
    //    return "";
    //}

   //template<typename T>
   //typename std::enable_if<std::is_same<T, BLOB>::value, T>::type getErrorVal() {
   //    return {nullptr, 0};
   //}

private:
    bool JsonTransaction(const rapidjson::Document& doc) {
        begin();

        for(size_t i = 0, size = doc.Size(); i < size; i++) {
            if(!executeJson(doc[i])) {
                std::cout << "execute json error" << std::endl;
                rollback();
                break;
            } else {
                //std::cout << "execute json success" << std::endl;
            }
        }
        //if(m_code != SQLITE_OK) {
        //    return false;
        //}

        commit();

        return true;
    }

private:
    int closeDbHandle() {
        int code = sqlite3_close(m_handle);

        while(code == SQLITE_BUSY) {    //循环释放所有的stmt
            code = SQLITE_OK;
            auto stmt = sqlite3_next_stmt(m_handle, NULL);
            if(stmt == nullptr) {
                break;
            }

            code = sqlite3_finalize(stmt);
            if(code == SQLITE_OK) {
                code = sqlite3_close(m_handle);
            }
        }
        return code;
    }
};
}

