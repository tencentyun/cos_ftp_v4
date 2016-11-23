#ifndef FILE_DOWNLOAD_TASK_H
#define FILE_DOWNLOAD_TASK_H
#pragma once

#include <pthread.h>
#include <string>
#include "op/BaseTask.h"
#include "op/BaseOp.h"
#include "util/FileUtil.h"
#include "util/HttpUtil.h"
#include "util/CodecUtil.h"
#include "util/HttpSender.h"
#include "util/StringUtil.h"
#include "request/CosResult.h"
#include "CosParams.h"
#include "CosDefines.h"
#include "CosConfig.h"
#include "CosSysConfig.h"

using std::string;
namespace qcloud_cos{

class FileDownloadTask : public BaseTask {

public:
    FileDownloadTask(uint64_t appid, const string &bucket, const string &down_url);
    ~FileDownloadTask(){};
    void run();
    void setRangeStart(uint64_t rangeStart);
    void setRangeEnd(uint64_t rangeEnd);
    void setDownBuf(char* downbuf);
    bool taskSuccess();
    uint64_t getDownloadSize();

private:
    uint64_t m_appid;
    string m_bucket;
    string m_url;
    uint64_t m_range_start;
    uint64_t m_range_end;
    uint64_t m_download_size;
    char* m_downbuf;
    int m_retcode;
};

}


#endif
