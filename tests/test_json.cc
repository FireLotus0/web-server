#include "jsoncpp.h"

#include <iostream>

void testJson() {
    HSQ::JsonCpp jcp;
    jcp.StartArray();
    for(int i =0; i < 100; i++) {
        jcp.StartObject();
        jcp.WriteJson(std::string("ID"), i);
        jcp.WriteJson(std::string("Name"), "jack");
        jcp.WriteJson(std::string("Address"), "shandong");
        jcp.EndObject();
    }
    jcp.EndArray();

    std::cout << "res is: " << std::endl;
    std::cout << jcp.GetString();
}

int main() {

    testJson();
    return 0;
}