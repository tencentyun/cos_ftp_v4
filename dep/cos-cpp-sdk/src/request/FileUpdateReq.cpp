#include "request/FileUpdateReq.h"
#include "request/ReqBase.h"
#include "util/FileUtil.h"
#include "CosParams.h"
#include "json/json.h"
#include <string.h>
#include <fstream>
#include <string>

using std::string;
using std::pair;
namespace qcloud_cos {

FileUpdateReq::FileUpdateReq(string& bucket, string& path) : ReqBase(bucket)
{
    this->path = path;
    this->biz_attr = "";
    this->authority = "";
    this->forbid = 0;
}

void FileUpdateReq::setBizAttr(string& biz_attr)
{
    this->biz_attr = biz_attr;
    this->flag |= FLAG_BIZ_ATTR;
}

void FileUpdateReq::setAuthority(string& authority)
{
    this->authority = authority;
    this->flag |= FLAG_AUTHORITY;
}

void FileUpdateReq::setForbid(int forbid)
{
    this->forbid = forbid;
    this->flag |= FLAG_FORBID;
}

bool FileUpdateReq::isUpdateBizAttr()
{
    return this->flag && FLAG_BIZ_ATTR;
}
bool FileUpdateReq::isUpdateForBid()
{
    return this->flag && FLAG_FORBID;
}
bool FileUpdateReq::isUpdateAuthority()
{
    return this->flag && FLAG_AUTHORITY;
}
bool FileUpdateReq::isUpdateCustomHeader()
{
    return this->flag && FLAG_CUSTOM_HEADER;
}

void FileUpdateReq::setCustomHeader(map<string,string>& custom_header)
{
    map<string, string>::iterator iter;
    for(iter=custom_header.begin(); iter != custom_header.end(); iter++)
    {
        if (isCustomHeader(iter->first))
        {
            this->custom_headers.insert(std::pair<string,string>(iter->first, iter->second));
        }
    }

    this->flag |= FLAG_CUSTOM_HEADER;
}

string FileUpdateReq::getFilePath()
{
    return this->path;
}

string FileUpdateReq::getFormatPath()
{
    return FileUtil::FormatFilePath(this->path);
}

bool FileUpdateReq::isParaValid(CosResult &cosResult)
{
    if (FileUtil::IsRootPath(getFilePath())) 
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(CAN_NOT_OP_ROOT_PATH);
        return false;
    }

    if (!isLegalFilePath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }

    if (!isAuthorityValid())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage("parameter authority error");
        return false;
    }

    return true;
}

bool FileUpdateReq::isLegalFilePath()
{
    return FileUtil::IsLegalFilePath(this->path);
}

int FileUpdateReq::getForbid()
{
    return this->forbid;
}

int FileUpdateReq::getFlag()
{
    return this->flag;
}

string FileUpdateReq::getBizAttr()
{
    return this->biz_attr;
}

string FileUpdateReq::getAuthority()
{
    return this->authority;
}
    
map<string,string>& FileUpdateReq::getCustomHeaders()
{
    return this->custom_headers;
}

bool FileUpdateReq::isAuthorityValid()
{
    if (this->authority.empty() 
        || this->authority == "eInvalid"
        || this->authority == "eWRPrivate"
        || this->authority == "eWPrivateRPublic"){
        return true;
    }

    return false;
}

bool FileUpdateReq::isCustomHeader(const string& head)
{
    if (0 == head.compare(PARA_CACHE_CONTROL) ||
        0 == head.compare(PARA_CONTENT_TYPE) ||
        0 == head.compare(PARA_CONTENT_DISPOSITION) ||
        0 == head.compare(PARA_CONTENT_LANGUAGE) ||
        0 == head.compare(PARA_CONTENT_ENCODING) ||
        0 == head.compare(0,strlen(PARA_X_COS_META_PREFIX.c_str()),PARA_X_COS_META_PREFIX)){
        return true;
    }

    return false;
}

string FileUpdateReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFilePath();
    root["biz_attr"] = getBizAttr();
    root["authority"] = getAuthority();
    root["forbid"] = getForbid();

    Json::FastWriter writer;
    return writer.write(root);
}

}

