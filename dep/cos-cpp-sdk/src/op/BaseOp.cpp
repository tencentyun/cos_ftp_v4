#include "op/BaseOp.h"
#include "util/FileUtil.h"
#include "util/StringUtil.h"
#include "util/HttpUtil.h"
#include "util/HttpSender.h"
#include "CosDefines.h"
#include "CosParams.h"
#include "Auth.h"
#include <stdint.h>
#include <string>
#include <map>
#include "glog/logging.h"

using std::string;
using std::map;
namespace qcloud_cos{

CosConfig& BaseOp::getCosConfig()
{
    return this->config;
}

uint64_t BaseOp::getAppid()
{
    return config.getAppid();
}

string BaseOp::getSecretID()
{
    return config.getSecretId();
}

string BaseOp::getSecretKey()
{
    return config.getSecretKey();
}

void BaseOp::printOpResult(const string &op_name, const string &result) {
    Json::Value resultJson = StringUtil::StringToJson(result);
    if (resultJson["code"] != 0) {
        LOG(ERROR) << op_name << ", op_result: " << result;
    } else {
        LOG(INFO) << op_name << ", op_result: " << result;
    }
}

string BaseOp::statBase(const string &bucket,const string &path) 
{
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, path);

    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, bucket);

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_STAT;
    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    return HttpSender::SendGetRequest(fullUrl, http_headers, http_params);
}

string BaseOp::delBase(const string &bucket,const string &path) {
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, path);
    string formatPath = FileUtil::FormatPath(path);
    string sign = Auth::AppSignOnce(this->getAppid(),this->getSecretID(),
                                this->getSecretKey(),formatPath, bucket);

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_DELETE;

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    return HttpSender::SendJsonPostRequest(fullUrl, http_headers, http_params);
}

}
