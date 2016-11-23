#include "request/ReqBase.h"
#include "request/FileDownloadReq.h"
#include "util/FileUtil.h"
#include "util/CodecUtil.h"
#include "util/HttpUtil.h"
#include "util/StringUtil.h"
#include "CosDefines.h"
#include "CosSysConfig.h"
#include <string>

using std::string;
namespace qcloud_cos {

FileDownloadReq::FileDownloadReq(string& bucket,string& filePath) : ReqBase(bucket)
{
    this->_filePath = StringUtil::Trim(filePath);
}

FileDownloadReq::FileDownloadReq(const FileDownloadReq& req) : ReqBase(req.msBucket)
{
    this->_filePath = req._filePath;
}

bool FileDownloadReq::isParaValid(CosResult& cosResult)
{
    if (!this->_filePath.empty() && !isLegalFilePath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }
    return true;
}

string FileDownloadReq::getFilePath()
{
    return this->_filePath;
}

string FileDownloadReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["filePath"] = getFilePath();

    Json::FastWriter writer;
    return writer.write(root);
}

}

