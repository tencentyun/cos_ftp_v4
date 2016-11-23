#include "util/StringUtil.h"
#include <iostream>
#include <stdio.h>

using std::cerr;

namespace qcloud_cos {

string& StringUtil::Trim(string &s) {
    if (s.empty()) {
        return s;
    }

    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

string StringUtil::JsonToString(const Json::Value &jsonObject) {
    Json::FastWriter fast_writer;
    return fast_writer.write(jsonObject);
}

Json::Value StringUtil::StringToJson(const string &jsonStr) {
    Json::Reader reader;
    Json::Value jsonObject;
    if (!reader.parse(jsonStr, jsonObject)) {
        std::cerr << "parse string to json error, origin str:" << jsonStr << std::endl;
    }
    return jsonObject;
}

string StringUtil::Uint64ToString(uint64_t num) {
    char buf[65];
#if __WORDSIZE == 64
    snprintf(buf, sizeof(buf), "%lu", num);
#else
    snprintf(buf, sizeof(buf), "%llu", num);
#endif
    string str(buf);
    return str;
}

string StringUtil::IntToString(int num) {
    char buf[65];
    snprintf(buf, sizeof(buf), "%d", num);
    string str(buf);
    return str;
}

}