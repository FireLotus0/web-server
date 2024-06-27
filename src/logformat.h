#pragma once

#include<list>
#include<ostream>
#include<unordered_map>
#include<memory>
#include<iomanip>
#include<mutex>

#include"logevent.h"
#include"util.h"

namespace HSQ
{
//*************************************************************************

//FormatItem:抽象类---实现继承---解决抽象问题
class FormatItem
{
public:
    using ptr=std::shared_ptr<FormatItem>;
    virtual void format(std::ostream& os,HSQ::LogEvent::ptr event)=0;
    virtual ~FormatItem(){}
};
//日志内容---%m
class MessageItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//日志级别---%p
class LevelItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//日志产生时间---%d
class DateTimeItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//事件用户名---%c
class UserNameItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//输出Tab---%T
class TabItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//输出程序运行到产生日志事件时经过的毫秒数---%r
class ElapsedMsItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};

//获取文件名---%f
class FileNameItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//获取线程名---%t
class ThreadNameItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//获取线程ID---%N
class ThreadIdItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//获取协程ID---%F
class FiberIdItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//获取行号---%l
class LineItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//输出换行---%n
class NewLineItem:public FormatItem
{
public:
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//输出其他字符：[]
class OtherItem:public FormatItem
{
private:
    char m_ch;
public:
    OtherItem(char ch);
    void format(std::ostream& os,HSQ::LogEvent::ptr event)override;
};
//*************************************************************************
//静态格式项map，这样每一种format只需要从这里取即可，不需要再创建
//但是，全局数据的访问在多线程环境中存在数据竞争。
//这里，配置一般很少会访问修改，每一个LogFormat只会在解析pattern的时候进行访问
//如果要将其做成线程局部的，则会浪费很多堆空间。相比之下，用一个全局的读写锁进行保护即可。



//*************************************************************************
//%d%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
class LogFormat
{
public:
    using ptr=std::shared_ptr<LogFormat>;
    LogFormat();
    void init();
    LogFormat(const std::string &pattern);
    void SetPattern(const std::string &pattern);
    std::string format(HSQ::LogEvent::ptr event);

private:
    //c++11允许非静态数据成员在类中赋初值
    std::string m_pattern="%d%T%t%T%N%T%F%T%T[%p]%T%T[%c]%T%f:%l%T%m%n";
    std::list<FormatItem::ptr> m_items;   //多态
};


class Item_Map
{
public:
    Item_Map();
    std::shared_ptr<FormatItem> GetItem(char ch);

private:
    std::mutex mt;
    std::unordered_map<char,FormatItem::ptr> item_map;
};

//*************************************************************************
}