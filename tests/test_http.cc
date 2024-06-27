#include "http/http.h"
#include "logger.h"

void test_request() {
	HSQ::http::HttpRequest::ptr req(new HSQ::http::HttpRequest);
	req->setHeader("host", "www.baidu.com");
	req->setBody("hello baidu");
	req->dump(std::cout) << std::endl;
	
}

void test_response() {
	HSQ::http::HttpResponse::ptr rsp(new HSQ::http::HttpResponse);
	rsp->setHeader("xx", "baidu");
	rsp->setBody("hello baidu");
	rsp->dump(std::cout) << std::endl;
}

int main() {
	test_request();
	test_response();
	return 0;
}
