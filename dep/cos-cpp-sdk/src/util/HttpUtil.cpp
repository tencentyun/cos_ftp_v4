#include "util/Sha1.h"
#include "util/HttpUtil.h"
#include "util/FileUtil.h"
#include "CosSysConfig.h"
#include "CosDefines.h"
#include "util/l5_endpoint_provider.h"
#include <string>

using std::string;
namespace qcloud_cos {

string HttpUtil::GetEncodedCosUrl(const string& endpoint,
                                      const string& bucketName,
                                      const string& dstPath,
                                      uint64_t  appid) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
             "/%lu/%s/%s",
#else
             "/%lu/%s/%s",
#endif
             appid,
             bucketName.c_str(),
             dstPath.c_str());

    string url_str(urlBytes);
    return endpoint + FileUtil::EncodePath(url_str);
}

string HttpUtil::GetEncodedCosUrl(uint64_t appid, const std::string& bucket_name,
                                     const std::string& path)
{
    return GetEncodedCosUrl(CosSysConfig::getUploadDomain(), bucket_name, path, appid);
}

string HttpUtil::GetEncodedDownloadCosUrl(
                    const string& domain,
                    const string& dstPath,
                    const string& sign) 
{
    string url_str = domain + dstPath;
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

string HttpUtil::GetEncodedDownloadCosUrl(
                    uint64_t  appid,
                    const string& bucketName,
                    const string& dstPath,
                    const string& domain,
                    const string& sign) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
     "%s-%lu.%s/%s",
#else
     "%s-%lu.%s/%s",
#endif
     bucketName.c_str(),
     appid,
     domain.c_str(),
     dstPath.c_str()
    );

    string url_str(urlBytes);
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

string HttpUtil::GetEncodedDownloadCosCdnUrl(
                    uint64_t  appid,
                    const string& bucketName,
                    const string& dstPath,
                    const string& domain,
                    const string& sign) 
{
    char urlBytes[10240];
    snprintf(urlBytes, sizeof(urlBytes),
#if __WORDSIZE == 64
     "%s/files/v2/%lu/%s/%s",
#else
     "%s/files/v2/%lu/%s/%s",
#endif
     domain.c_str(),
     appid,
     bucketName.c_str(),
     dstPath.c_str()
    );

    string url_str(urlBytes);
    string url = "http:/" + FileUtil::EncodePath(url_str) + "?sign=" + sign;
    return url;
}

}
