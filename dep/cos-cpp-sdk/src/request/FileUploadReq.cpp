#include "request/FileUploadReq.h"
#include "request/ReqBase.h"
#include "util/FileUtil.h"
#include "util/CodecUtil.h"
#include "CosParams.h"
#include "CosDefines.h"
#include "CosSysConfig.h"
#include "json/json.h"
#include <fstream>
#include <string>

using std::string;
namespace qcloud_cos {

FileUploadReq::FileUploadReq(string& bucket,string& srcPath, string& dstPath, int insertOnly) : ReqBase(bucket)
{
    this->srcPath = srcPath;
    this->dstPath = dstPath;
    this->biz_attr = "";
    this->insertOnly = insertOnly;
    this->slice_size = CosSysConfig::getSliceSize();
    this->buffer = NULL;
    this->bufferLen = 0;
}

FileUploadReq::FileUploadReq(string& bucket,string& dstPath, const char* pbuffer, uint64_t buflen, int insertOnly) : ReqBase(bucket)
{
    this->srcPath = "";
    this->dstPath = dstPath;
    this->biz_attr = "";
    this->insertOnly = insertOnly;
    this->slice_size = CosSysConfig::getSliceSize();
    this->buffer = pbuffer;
    this->bufferLen = buflen;
}

FileUploadReq::FileUploadReq(const FileUploadReq &req) : ReqBase(req.msBucket)
{
    this->srcPath = req.srcPath;
    this->dstPath = req.dstPath;
    this->biz_attr = req.biz_attr;
    this->insertOnly = req.insertOnly;
    this->slice_size = req.slice_size;
    this->buffer = req.buffer;
    this->bufferLen = req.bufferLen; 
}

bool FileUploadReq::isBufferUpload()
{
    return (this->buffer == NULL ? false : true);
}

string FileUploadReq::getSha()
{
    return this->sha;
}

void FileUploadReq::setSha(string& sha)
{
    this->sha = sha;
}

const char* FileUploadReq::getBuffer()
{
    return this->buffer;
}

uint64_t FileUploadReq::getBufferLen()
{
    return this->bufferLen;
}

uint64_t FileUploadReq::getFileSize()
{
    if (!this->srcPath.empty() && this->buffer == NULL){
        return FileUtil::getFileLen(this->srcPath);
    } else if (this->buffer != NULL) {
        return this->bufferLen;
    } else {
        return 0;
    }
}

bool FileUploadReq::isParaValid(CosResult &cosResult)
{
    if (!isBufferUpload()  && !FileUtil::isFileExists(this->srcPath))
    {
        cosResult.setCode(LOCAL_FILE_NOT_EXIST_CODE);
        cosResult.setMessage(LOCAL_FILE_NOT_EXIST_DESC);
        return false;
    }
    
    if (!isLegalFilePath())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage(PARA_PATH_ILEAGEL);
        return false;
    }

    if (!isBufferUpload() && !isSliceSizeValid())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage("parameter slice error, only support 512K,1M,2M,3M");
        return false;
    }

    if (!isBufferUpload() && !isSliceSizeAndShaValid())
    {
        cosResult.setCode(PARA_ERROR_CODE);
        cosResult.setMessage("not support take sha when slice not equal 1M, error");
        return false;
    }
    
    return true;
}

string FileUploadReq::getFormatPath()
{
    return FileUtil::FormatFilePath(this->dstPath);
}

bool FileUploadReq::isLegalFilePath()
{
    return FileUtil::IsLegalFilePath(this->dstPath);
}

void FileUploadReq::setBizAttr(string& biz_atrr)
{
    this->biz_attr = biz_atrr;
}

void FileUploadReq::setInsertOnly(int insertOnly)
{
    this->insertOnly = insertOnly;
}

void FileUploadReq::setSliceSize(int sliceSize)
{
    this->slice_size = sliceSize;
}

bool FileUploadReq::isSliceSizeValid()
{
    if (this->slice_size == SLICE_SIZE_512K
        || this->slice_size == SLICE_SIZE_1M
        || this->slice_size == SLICE_SIZE_2M
        || this->slice_size == SLICE_SIZE_3M
        )
    {
        return true;
    }
    return false;
}

bool FileUploadReq::isSliceSizeAndShaValid()
{
    if (this->slice_size != SLICE_SIZE_1M && CosSysConfig::isTakeSha())
    {
        return false;
    }
    return true;
}

int FileUploadReq::getSliceSize()
{
    return this->slice_size;
}

string FileUploadReq::getBucket()
{
    return this->msBucket;
}
string FileUploadReq::getSrcPath()
{
    return this->srcPath;
}
string FileUploadReq::getDstPath()
{
    return this->dstPath;
}
string FileUploadReq::getBizAttr()
{
    return this->biz_attr;
}
int FileUploadReq::getInsertOnly()
{
    return this->insertOnly;
}

string FileUploadReq::toJsonString()
{
    Json::Value root;
    root["bucket"] = getBucket();
    root["srcPath"] = getSrcPath();
    root["dstPath"] = getDstPath();
    root["biz_attr"] = getBizAttr();
    root["insertOnly"] = getInsertOnly();

    Json::FastWriter writer;
    return writer.write(root);
}

}

