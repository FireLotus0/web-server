#pragma once

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <cxxabi.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include "http.h"
#include "httpsession.h"

namespace HSQ {
namespace http {

template<class T>
const char* TypeToName() {
    static const char* s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
    return s_name;
}


/**
 * @brief Servlet封装
 */
class Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        :m_name(name) {}

    virtual ~Servlet() {}


    virtual int32_t handle(HSQ::http::HttpRequest::ptr request
                   , HSQ::http::HttpResponse::ptr response
                   , HSQ::http::HttpSession::ptr session) = 0;
                   
    const std::string& getName() const { return m_name;}
protected:
    /// 名称
    std::string m_name;
};


class FunctionServlet : public Servlet {
public:
    /// 智能指针类型定义
    typedef std::shared_ptr<FunctionServlet> ptr;
    /// 函数回调类型定义
    typedef std::function<int32_t (HSQ::http::HttpRequest::ptr request
                   , HSQ::http::HttpResponse::ptr response
                   , HSQ::http::HttpSession::ptr session)> callback;


    FunctionServlet(callback cb);

    virtual int32_t handle(HSQ::http::HttpRequest::ptr request
                   , HSQ::http::HttpResponse::ptr response
                   , HSQ::http::HttpSession::ptr session) override;
private:

    callback m_cb;
};

class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> ptr;
    virtual ~IServletCreator() {}
    virtual Servlet::ptr get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<HoldServletCreator> ptr;
    HoldServletCreator(Servlet::ptr slt)
        :m_servlet(slt) {
    }

    Servlet::ptr get() const override {
        return m_servlet;
    }

    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    Servlet::ptr m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> ptr;

    ServletCreator() {
    }

    Servlet::ptr get() const override {
        return Servlet::ptr(new T);
    }

    std::string getName() const override {
        return TypeToName<T>();
    }
};

class ServletDispatch : public Servlet {
public:

    typedef std::shared_ptr<ServletDispatch> ptr;

    ServletDispatch();
    virtual int32_t handle(HSQ::http::HttpRequest::ptr request
                   , HSQ::http::HttpResponse::ptr response
                   , HSQ::http::HttpSession::ptr session) override;


    void addServlet(const std::string& uri, Servlet::ptr slt);

    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    void addGlobServlet(const std::string& uri, Servlet::ptr slt);

    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void addServletCreator(const std::string& uri, IServletCreator::ptr creator);
    void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);

    template<class T>
    void addServletCreator(const std::string& uri) {
        addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    template<class T>
    void addGlobServletCreator(const std::string& uri) {
        addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    void delServlet(const std::string& uri);

    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default;}

    void setDefault(Servlet::ptr v) { m_default = v;}

    Servlet::ptr getServlet(const std::string& uri);

    Servlet::ptr getGlobServlet(const std::string& uri);

    Servlet::ptr getMatchedServlet(const std::string& uri);

private:
    std::mutex m_mutex;

    std::unordered_map<std::string, IServletCreator::ptr> m_datas;

    std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;
    
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet {
public:

    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet(const std::string& name);
    virtual int32_t handle(HSQ::http::HttpRequest::ptr request
                   , HSQ::http::HttpResponse::ptr response
                   , HSQ::http::HttpSession::ptr session) override;

private:
    std::string m_name;
    std::string m_content;
};

}
}


