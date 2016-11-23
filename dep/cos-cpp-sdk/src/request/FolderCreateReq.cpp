#include "request/FolderCreateReq.h"
#include "util/FileUtil.h"
#include "json/json.h"
#include <string>

using std::string;
namespace qcloud_cos {

bool FolderCreateReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFolderPath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }

    return true;
}

bool FolderCreateReq::isLegalFolderPath()
{
    return FileUtil::isLegalFolderPath(this->path);
}

string FolderCreateReq::getFormatFolderPath()
{
    return FileUtil::FormatFolderPath(this->path);
}

string FolderCreateReq::getBizAttr()
{
    return this->biz_attr;
}

string FolderCreateReq::getFolderPath()
{
    return this->path;
}

string FolderCreateReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getFolderPath();
    root["biz_attr"] = getBizAttr();

    Json::FastWriter writer;
    return writer.write(root);
}

}
