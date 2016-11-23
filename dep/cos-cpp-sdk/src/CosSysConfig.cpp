#include "CosSysConfig.h"
#include "CosDefines.h"
#include <stdint.h>

namespace qcloud_cos {

//上传文件是否携带sha值,默认不携带
bool CosSysConfig::takeSha = false;
//上传文件分片大小,默认1M
uint64_t CosSysConfig::sliceSize = SLICE_SIZE_1M;
//签名超时时间,默认60秒
uint64_t CosSysConfig::expire_in_s = 300;
//HTTP连接时间(秒)
uint64_t CosSysConfig::timeout_in_ms = 5 * 1000;
//全局超时时间(毫秒)
uint64_t CosSysConfig::timeout_for_entire_request_in_ms = 300 * 1000;
//L5的模块ID信息
int64_t CosSysConfig::l5_modid = -1;
int64_t CosSysConfig::l5_cmdid = -1;

//下载域名类型,用于构造下载url,默认为cdn
bool  CosSysConfig::use_cdn = false;
string  CosSysConfig::downloadDomain = "cossh.myqcloud.com";

string  CosSysConfig::uploadDomain = "http://sh.file.myqcloud.com/files/v2/";

void CosSysConfig::setIsTakeSha(bool isTakeSha)
{
    takeSha = isTakeSha;
}

void CosSysConfig::setSliceSize(uint64_t slice_size)
{
    sliceSize = slice_size;
}

void CosSysConfig::setExpiredTime(uint64_t time)
{
    expire_in_s = time;
}
void CosSysConfig::setTimeoutInms(uint64_t time)
{
    timeout_in_ms = time;
}
void CosSysConfig::setGlobalTimeoutInms(uint64_t time)
{
    timeout_for_entire_request_in_ms = time;
}
void CosSysConfig::setL5Modid(int64_t modid)
{
    l5_modid = modid;
}
void CosSysConfig::setL5Cmdid(int64_t cmdid)
{
    l5_cmdid = cmdid;
}

void CosSysConfig::setDownloadDomain(bool download_use_cdn) {
    use_cdn = download_use_cdn;
    if (download_use_cdn) {
        downloadDomain = "file.myqcloud.com";
    }
}

void CosSysConfig::setCosRegion(const string &region) {
    uploadDomain.clear();
    uploadDomain.append("http://").append(region).append(".file.myqcloud.com/files/v2/");
    if (!use_cdn) {
        downloadDomain.clear();
        downloadDomain.append("cos").append(region).append(".myqcloud.com");
    }
}

string CosSysConfig::getDownloadDomain()
{
    return downloadDomain;
}

string CosSysConfig::getUploadDomain() {
    return uploadDomain;
}

bool CosSysConfig::isTakeSha()
{
    return takeSha;
}

uint64_t CosSysConfig::getSliceSize()
{
    return sliceSize;
}

uint64_t CosSysConfig::getExpiredTime()
{
    return expire_in_s;
}

uint64_t CosSysConfig::getTimeoutInms()
{
    return timeout_in_ms;
}
uint64_t CosSysConfig::getGlobalTimeoutInms()
{
    return timeout_for_entire_request_in_ms;
}
int64_t CosSysConfig::getL5Modid()
{
    return l5_modid;
}
int64_t CosSysConfig::getL5Cmdid()
{
    return l5_cmdid;
}
}

