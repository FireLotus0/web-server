#include <assert.h>
#include"scheduler.h"
#include"macro.h"
#include<iostream>


 static auto g_logger=GET_LOGGER("system");

 void test_fiber() {
   INFO(g_logger, "test_fiber call");
   static int s_count = 5;
    std::string msg = "test in fiber s_count=" + std::to_string(s_count);
    INFO(g_logger, msg);

//    sleep(1);
    if(--s_count >= 0) {
		assert(HSQ::Scheduler::GetThis() != nullptr);
        HSQ::Scheduler::GetThis()->Schedule(&test_fiber, HSQ::GetThreadId());
    }
}

int main(int argc, char** argv) {
    INFO(g_logger,  "main");
    HSQ::Scheduler sc(2, true, "test");
    //sc.Start();
    sleep(2);
    INFO(g_logger,  "schedule");
    sc.Schedule(&test_fiber);
  	sc.Start();
  // sc.Stop();
    //sleep(100000);
	sc.Stop();
    INFO(g_logger, "over");
    return 0;
}

