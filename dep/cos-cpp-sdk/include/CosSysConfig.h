#ifndef COS_SYS_CONF_H
#define COS_SYS_CONF_H
#include "CosDefines.h"
#include <stdint.h>
#include <pthread.h>
#include "curl/curl.h"

namespace qcloud_cos{

class CosSysConfig {
public:
    //设置签名超时时间,单位:秒
    static void setExpiredTime(uint64_t time);        
    //设置连接超时时间,单位:豪秒
    static void setTimeoutInms(uint64_t time);
    //设置全局超时时间,单位:豪秒
    static void setGlobalTimeoutInms(uint64_t time);
    //设置上传分片大小,默认1M
    static void setSliceSize(uint64_t sliceSize);
    //设置L5模块MODID
    static void setL5Modid(int64_t l5_modid);
    //设置L5模块CMDID
    static void setL5Cmdid(int64_t l5_cmdid);
    //设置上传文件是否携带sha值,默认不携带
    static void setIsTakeSha(bool takeSha);
    //设置下载域名:cos,cdn,innercos,自定义,默认:cos
    static void setDownloadDomain(bool download_use_cdn = false);
    //  设置上传的region
    static void setCosRegion(const string &region);
    static uint64_t getExpiredTime();
    static uint64_t getTimeoutInms();
    static uint64_t getGlobalTimeoutInms();
    static uint64_t getSliceSize();
    static int64_t getL5Modid();
    static int64_t getL5Cmdid();
    static bool isTakeSha();
    static string getDownloadDomain();
    static string getUploadDomain();
private:
    static bool takeSha;
    //上传分片大小
    static uint64_t sliceSize;
    //签名超时时间(秒)
    static uint64_t expire_in_s;
    // 超时时间(毫秒)
    static uint64_t timeout_in_ms;
    // 全局超时时间(毫秒)
    static uint64_t timeout_for_entire_request_in_ms;
    static int64_t l5_modid;
    static int64_t l5_cmdid;
    // 下载是否通过CDN
    static bool use_cdn;
    //下载域名
    static string  downloadDomain;
    // 上传域名
    static string uploadDomain;
};
}

#endif
