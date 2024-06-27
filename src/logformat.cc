#include<mutex>
#include<iostream>
#include"logformat.h"

namespace HSQ
{
//日志内容---%m
void MessageItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(4)<<event->GetEventMessage();
}

//日志级别---%p
void LevelItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<event->GetEventLevel();
}  
//日志产生时间---%d
void DateTimeItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(8)<<event->GetEventTime();
}
//事件用户名---%c
void UserNameItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<event->GetEventUserName();
}
//输出Tab---%T
void TabItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<"\t";
}
//输出程序运行到产生日志事件时经过的毫秒数---%r
void ElapsedMsItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(8)<<event->GetEventElapsedMs();
}
//获取文件名---%f
void FileNameItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<event->GetEventFileName();
}
//获取线程名---%t
void ThreadNameItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(4)<<event->GetEventThreadName();
}
//获取线程ID---%N
void ThreadIdItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(8)<<event->GetEventThreadId();
}
//获取协程ID---%F
void FiberIdItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(6)<<"   "<<event->GetEventFiberId();
}
//获取行号---%l
void LineItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::setiosflags(std::ios::left | std::ios::fixed)<<std::setw(8)<<event->GetEventLine();
}
//输出换行---%n
void NewLineItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<std::endl;
}
//输出其他字符：[]
OtherItem::OtherItem(char ch):FormatItem(),m_ch(ch){}
void OtherItem::format(std::ostream& os,HSQ::LogEvent::ptr event)
{
    os<<m_ch;
}



// case 'm': return std::make_shared<MessageItem>(new MessageItem());
// case 'p': return std::make_shared<LevelItem>(new LevelItem());
// case 'd': return std::make_shared<DateTimeItem>(new DateTimeItem());
// case 'c': return std::make_shared<UserNameItem>(new UserNameItem());
// case 'T': return std::make_shared<TabItem>(new TabItem());
// case 'r': return std::make_shared<ElapsedMsItem>(new ElapsedMsItem());
// case 'f': return std::make_shared<FileNameItem>(new FileNameItem());
// case 't': return std::make_shared<ThreadNameItem>(new ThreadNameItem());
// case 'N': return std::make_shared<ThreadIdItem>(new ThreadIdItem());
// case 'F': return std::make_shared<FiberIdItem>(new FiberIdItem());
// case 'l': return std::make_shared<LineItem>(new LineItem());
// case 'n': return std::make_shared<NewLineItem>(new NewLineItem());
// case '[': return std::make_shared<OtherItem>(new OtherItem('['));
// case ']': return std::make_shared<OtherItem>(new OtherItem(']'));
//static std::mutex item_map_mt;
//static std::shared_mutex item_map_mt;
Item_Map::Item_Map()
{
    item_map=
    {
         {'m',std::shared_ptr<MessageItem>(new MessageItem())},
        {'p',std::shared_ptr<LevelItem>(new LevelItem())},
        {'d',std::shared_ptr<DateTimeItem>(new DateTimeItem())},
        {'c',std::shared_ptr<UserNameItem>(new UserNameItem())},
        {'T',std::shared_ptr<TabItem>(new TabItem())},
        {'r',std::shared_ptr<ElapsedMsItem>(new ElapsedMsItem())},
        {'f',std::shared_ptr<FileNameItem>(new FileNameItem())},
        {'t',std::shared_ptr<ThreadNameItem>(new ThreadNameItem())},
        {'N',std::shared_ptr<ThreadIdItem>(new ThreadIdItem())},
        {'F',std::shared_ptr<FiberIdItem>(new FiberIdItem())},
        {'l',std::shared_ptr<LineItem>(new LineItem())},
        {'n',std::shared_ptr<NewLineItem>(new NewLineItem())},
        {'[',std::shared_ptr<OtherItem>(new OtherItem('['))},
        {']',std::shared_ptr<OtherItem>(new OtherItem(']'))},
        {':',std::shared_ptr<OtherItem>(new OtherItem(':'))}
    };
}
std::shared_ptr<FormatItem> Item_Map::GetItem(char ch)
{
    //ASSERT(item_map.size()>0,"Error item_map==0");
    std::lock_guard<std::mutex> lock(mt);
    auto it=item_map.find(ch);
    if(it!=item_map.end())
        return it->second;
    return nullptr;
}

LogFormat::LogFormat()
{
    init();
}

void LogFormat::init()
{
    for(auto& i:m_pattern)
    {
        if(i=='%')
            continue;
        auto iter=HSQ::Singleton<HSQ::Item_Map>::GetInstance()->GetItem(i);
        if(iter!=nullptr)
            m_items.push_back(iter);
    }
}
LogFormat::LogFormat(const std::string &pattern)
{
    m_pattern=pattern;
    init();
}
void LogFormat::SetPattern(const std::string &pattern)
{
    m_pattern=pattern;
}
std::string LogFormat::format(HSQ::LogEvent::ptr event)
{
    std::stringstream ss;
    for(auto &&i:m_items)   //自动类型推导即可
        i->format(ss,event);
    return ss.str();
}


//*************************************************************************
//namespace end   
}