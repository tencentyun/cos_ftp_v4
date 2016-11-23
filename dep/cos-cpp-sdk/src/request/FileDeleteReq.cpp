#include "request/FileDeleteReq.h"
#include "request/CosResult.h"
#include "util/FileUtil.h"
#include "json/json.h"
#include <string>

using std::string;
namespace qcloud_cos{

bool FileDeleteReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFilePath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }
    return true;
}

string FileDeleteReq::getFormatPath()
{
    return FileUtil::FormatFilePath(this->path);
}

string FileDeleteReq::getFilePath()
{
    return this->path;
}

string FileDeleteReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFilePath();

    Json::FastWriter writer;
    return writer.write(root);
}

}
