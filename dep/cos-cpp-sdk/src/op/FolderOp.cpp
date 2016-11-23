#include "op/FolderOp.h"
#include "util/CodecUtil.h"
#include "util/HttpUtil.h"
#include "util/StringUtil.h"
#include "util/FileUtil.h"
#include "request/CosResult.h"
#include "CosDefines.h"
#include "Auth.h"
#include <map>
#include <string>
#include "glog/logging.h"
using std::string;
using std::map;

namespace qcloud_cos{

//目录创建
string FolderOp::FolderCreate(FolderCreateReq& req)
{
    LOG(INFO) << "FolderCreate req: " << req.toJsonString();
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        LOG(ERROR) << "FolderCreate illegal req: " << req.toJsonString();
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string path =  req.getFormatFolderPath();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, path);
    uint64_t expired = FileUtil::GetExpiredTime();
    string sign = Auth::AppSignMuti(this->getAppid(),this->getSecretID(),
                                this->getSecretKey(),expired,bucket);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_CREATE;
    http_params[PARA_BIZ_ATTR] = req.getBizAttr();
    string createRet = HttpSender::SendJsonPostRequest(
        fullUrl, http_headers, http_params);
    printOpResult("FolderCreate", createRet);
    return createRet;
}

//目录查询
string FolderOp::FolderStat(FolderStatReq& req)
{
    LOG(INFO) << "FolderStat req: " << req.toJsonString();
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        LOG(ERROR) << "FolderStat illegal req: " << req.toJsonString();
        return ret.toJsonString();
    }

    string folderPath = req.getFolderPath();
    string statRet = statBase(req.getBucket(), folderPath);
    printOpResult("FolderStat", statRet);
    return statRet;
}

//目录列表(前缀匹配或者非前缀匹配)
string FolderOp::FolderList(FolderListReq& req)
{
    LOG(INFO) << "FolderList Req: " << req.toJsonString();
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        LOG(ERROR) << "FolderList illegal Req: " << req.toJsonString();
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string folderPath = FileUtil::FormatFolderPath(req.getListPath());
    uint64_t expired = FileUtil::GetExpiredTime();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, req.getListPath());
    string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                                expired,bucket);
    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_LIST;
    http_params[PARA_LIST_NUM] = StringUtil::IntToString(req.getListNum());
    http_params[PARA_LIST_FLAG] = StringUtil::IntToString(req.getListFlag());
    if (!req.getListContext().empty()){
        http_params[PARA_LIST_CONTEXT] = req.getListContext();
    }
    
    string listRet = HttpSender::SendGetRequest(fullUrl, http_headers, http_params);
    printOpResult("FolderList", listRet);
    return listRet;
}

//目录删除
string FolderOp::FolderDelete(FolderDeleteReq& req)
{
    LOG(INFO) << "FolderDelete req: " << req.toJsonString();
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        LOG(ERROR) << "FolderDelete illegal Req: " << req.toJsonString();
        return ret.toJsonString();
    }

    string deleteFolderRet = delBase(req.getBucket(), req.getFolderPath());
    printOpResult("FolderDelete", deleteFolderRet);
    return deleteFolderRet;
}

//目录更新
string FolderOp::FolderUpdate(FolderUpdateReq& req)
{
    LOG(INFO) << "FolderUpdate req: " << req.toJsonString();
    
    CosResult ret;
    if (!req.isParaValid(ret))
    {
        LOG(ERROR) << "FolderUpdate illegal Req: " << req.toJsonString();
        return ret.toJsonString();
    }

    string bucket = req.getBucket();
    string formatPath = req.getFormatFolderPath();
    string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);

    string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                                formatPath,bucket);

    std::map<string, string> http_headers;
    http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

    std::map<string, string> http_params;
    http_params[PARA_OP] = OP_UPDATE;
    http_params[PARA_BIZ_ATTR] = req.getBizAttr();

    string updateFolderRet = HttpSender::SendJsonPostRequest(
        fullUrl, http_headers, http_params);
    printOpResult("FolderUpdate", updateFolderRet);
    return updateFolderRet;
}

}

