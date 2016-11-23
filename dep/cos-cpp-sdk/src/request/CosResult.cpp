#include "request/CosResult.h"
#include "json/json.h"

namespace qcloud_cos{

CosResult::CosResult()
{}

CosResult::CosResult(int code, string message)
{
    this->code = code;
    this->message = message;
}

CosResult::~CosResult()
{}

void CosResult::setCode(int code)
{
    this->code = code;
}
void CosResult::setMessage(string message)
{
    this->message = message;
}

string CosResult::toJsonString()
{
    Json::Value root;
    root["code"] = this->code;
    root["message"] = this->message;

    Json::FastWriter fast_writer;
    return fast_writer.write(root);
}

}
