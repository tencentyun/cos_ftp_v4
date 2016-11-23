#include "request/FileMoveReq.h"
#include "util/FileUtil.h"
#include "json/json.h"
#include <string>

using std::string;
namespace qcloud_cos{

bool FileMoveReq::isParaValid(CosResult &cosResult)
{
    return true;
}

string FileMoveReq::getFormatPath()
{
    return FileUtil::FormatFilePath(this->srcPath);
}

int FileMoveReq::getOverWrite()
{
    return this->overwrite;
}

string FileMoveReq::getFilePath()
{
    return this->srcPath;
}

string FileMoveReq::getDstPath()
{
    return this->dstPath;
}

string FileMoveReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["srcPath"] = getFilePath();
    root["dstPath"] = getDstPath();
    root["overwrite"] = getOverWrite();

    Json::FastWriter writer;
    return writer.write(root);
}

}
