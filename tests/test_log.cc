#include"logappender.h"
#include"logevent.h"

#include"logger.h"
#include"loglevel.h"
#include"macro.h"
#include"util.h"
#include"logmanager.h"
#include"logformat.h"
#include<thread>
#include<vector>
#include<iostream>

static auto logger=GET_LOGGER("system");
int main(int argc,char** argv)
{
	
	DEBUG(logger,"hello InitProgram()");
	auto logger2=GET_LOGGER("root");
	auto logger3=GET_LOGGER("root");
	auto i=HSQ::Singleton<HSQ::LogManager>::GetInstance();
	std::cout<<"logger  map="<<i->GetLoggerCount();

}
