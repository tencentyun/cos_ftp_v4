#include "request/FileStatReq.h"
#include "util/FileUtil.h"
#include "json/json.h"
#include <string>

using std::string;
namespace qcloud_cos{

bool FileStatReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFilePath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }
    return true;
}

string FileStatReq::getFormatPath()
{
    return FileUtil::FormatFilePath(this->path);
}

string FileStatReq::getFilePath()
{
    return this->path;
}

string FileStatReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFilePath();

    Json::FastWriter writer;
    return writer.write(root);
}

}
