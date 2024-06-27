#pragma once

#include <string>
#include "writer.h"
#include "stringbuffer.h"
#include "document.h"
#include <type_traits>

using namespace rapidjson;

namespace HSQ
{
class JsonCpp
{
    using JsonWriter = Writer<StringBuffer>;

private:
    StringBuffer m_buf;
    JsonWriter m_writer;
    Document m_doc;

public:
    JsonCpp() : m_writer(m_buf) {}

    ~JsonCpp() {}

    void StartArray() {
        m_writer.StartArray();
    }

    void EndArray() {
        m_writer.EndArray();
    }

    void StartObject() {
        m_writer.StartObject();
    }

    void EndObject() {
        m_writer.EndObject();
    }

    template<typename T>
    void WriteJson(const std::string& key, T value) {
        m_writer.String(key.c_str());
        WriteValue(value);
    }

    const char* GetString() const {
        return m_buf.GetString();
    }

    void Clear() {
        m_buf.Clear();
    }

private:
    template<typename V>
    typename std::enable_if<std::is_same<V, int>::value>::type WriteValue(V val) {
        m_writer.Int(val);
    }

    template<typename V>
    typename std::enable_if<std::is_same<V, unsigned int>::value>::type WriteValue(V val) {
        m_writer.Uint(val);
    }

    template<typename V>
    typename std::enable_if<std::is_same<V, int64_t>::value>::type WriteValue(V val) {
        m_writer.Int64(val);
    }

    template<typename V>
    typename std::enable_if<std::is_floating_point<V>::value>::type WriteValue(V val) {
        m_writer.Double(val);
    }

    template<typename V>
    typename std::enable_if<std::is_same<V, bool>::value>::type WriteValue(V val) {
        m_writer.Bool(val);
    }

    template<typename V>
    typename std::enable_if<std::is_pointer<V>::value>::type WriteValue(V val) {
        m_writer.String(val);
    }

    template<typename V>
    typename std::enable_if<std::is_same<V, std::string>::value>::type WriteValue(V val) {
        m_writer.String(val.c_str());
    }

    template<typename V>
    typename std::enable_if<std::is_array<V>::value>::type WriteValue(V val) {
        m_writer.String(val);
    }

    template<typename V>
    typename std::enable_if<std::is_same<V, std::nullptr_t>::value>::type WriteValue(V val) {
        m_writer.Null();
    }


};
}