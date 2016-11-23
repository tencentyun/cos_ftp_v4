#ifndef COS_DEFINE_H
#define COS_DEFINE_H
//#include <syslog.h>
#include <string>
#include <stdio.h>
#include <syslog.h>
using std::string;
namespace qcloud_cos{

const string kPathDelimiter = "/";
const unsigned char kPathDelimiterChar = '/';
// 默认的外部地址
//const string kApiCosapiEndpoint = "http://web.file.myqcloud.com/files/v2/";
const string kApiCosapiEndpoint = "http://sh.file.myqcloud.com/files/v2/";
//const string kApiCosapiEndpoint = "http://180.163.3.250/files/v2/";
// 分片上传时，失败的最大重试次数
const int kMaxRetryTimes = 3;

const int THREAD_POOL_SIZE_UPLOAD_SLICE = 16;

const int SINGLE_FILE_SIZE = 8 * 1024 * 1024;
const int SLICE_SIZE_512K = 512 * 1024;
const int SLICE_SIZE_1M = 1 * 1024 * 1024;
const int SLICE_SIZE_2M = 2 * 1024 * 1024;
const int SLICE_SIZE_3M = 3 * 1024 * 1024;
const int DEFAULT_DOWN_RANGE_SIZE = 1 * 1024 * 1024;

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

}
#endif
