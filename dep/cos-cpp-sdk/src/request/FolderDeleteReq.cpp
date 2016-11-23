#include "request/FolderDeleteReq.h"
#include "util/FileUtil.h"
#include "json/json.h"

namespace qcloud_cos {

bool FolderDeleteReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFolderPath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }    
    return true;
}

bool FolderDeleteReq::isLegalFolderPath()
{
    return FileUtil::isLegalFolderPath(this->path);
}

string FolderDeleteReq::getFormatFolderPath()
{
    return FileUtil::FormatFolderPath(this->path);
}

string FolderDeleteReq::getFolderPath()
{
    return this->path;
}

string FolderDeleteReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFolderPath();

    Json::FastWriter writer;
    return writer.write(root);
}

}

