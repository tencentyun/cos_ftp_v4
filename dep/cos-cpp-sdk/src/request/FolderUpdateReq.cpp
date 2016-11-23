#include "request/FolderUpdateReq.h"
#include "util/FileUtil.h"
#include "json/json.h"

namespace qcloud_cos {

bool FolderUpdateReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFolderPath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }    
    return true;
}

bool FolderUpdateReq::isLegalFolderPath()
{
    return FileUtil::isLegalFolderPath(this->path);
}

string FolderUpdateReq::getFormatFolderPath()
{
    return FileUtil::FormatFolderPath(this->path);
}

string FolderUpdateReq::getBizAttr()
{
    return this->biz_attr;
}

string FolderUpdateReq::getFolderPath()
{
    return this->path;
}

string FolderUpdateReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFolderPath();
    root["biz_attr"] = getBizAttr();

    Json::FastWriter writer;
    return writer.write(root);
}

}
