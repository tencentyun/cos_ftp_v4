#include "request/FolderListReq.h"
#include "util/FileUtil.h"
#include "json/json.h"

namespace qcloud_cos {

bool FolderListReq::isParaValid(CosResult &cosResult)
{
    if (!isLegalFolderPath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }    
    return true;
}

bool FolderListReq::isLegalFolderPath()
{
    return FileUtil::isLegalFolderPath(this->path);
}

string FolderListReq::getFormatFolderPath()
{
    return FileUtil::FormatFolderPath(this->path);
}

int FolderListReq::getListNum()
{
    return this->num;
}

string FolderListReq::getListPath()
{
    return this->path;
}

int FolderListReq::getListFlag()
{
    return this->flag;
}

string FolderListReq::getListContext()
{
    return this->context;
}

string FolderListReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["path"] = getListPath();
    root["num"] = getListNum();    
    root["flag"] = getListFlag();
    root["context"] = getListContext();

    Json::FastWriter writer;
    return writer.write(root);
}

}
