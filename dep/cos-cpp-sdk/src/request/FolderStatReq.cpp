#include "request/FolderStatReq.h"
#include "util/FileUtil.h"
#include "json/json.h"

namespace qcloud_cos {

bool FolderStatReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFolderPath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }    
    return true;
}

bool FolderStatReq::isLegalFolderPath()
{
    return FileUtil::isLegalFolderPath(this->path);
}

string FolderStatReq::getFormatFolderPath()
{
    return FileUtil::FormatFolderPath(this->path);
}

string FolderStatReq::getFolderPath()
{
    return this->path;
}

string FolderStatReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFolderPath();

    Json::FastWriter writer;
    return writer.write(root);
}

}

