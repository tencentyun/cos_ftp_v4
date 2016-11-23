#include <string.h>
#include <stdint.h>
#include <map>
#include <string>
#include "op/FileOp.h"
#include "Auth.h"
#include "glog/logging.h"

using std::string;
using std::map;
namespace qcloud_cos{

    FileDownloadTask::FileDownloadTask(uint64_t appid, const string &bucket, const string &down_url) 
        : m_appid(appid), m_bucket(bucket), m_url(down_url) {}

    void FileDownloadTask::run()
    {
        std::map<string, string> user_params;
        std::map<string, string> http_params;
        char host_head[256];
        memset(host_head, 0, sizeof(host_head));
        snprintf(host_head, sizeof(host_head),"%s-%lu.%s", m_bucket.c_str(), 
                m_appid, CosSysConfig::getDownloadDomain().c_str());
        http_params["host"] = host_head;
        char range_head[128];
        memset(range_head, 0, sizeof(range_head));
        snprintf(range_head, sizeof(range_head),"bytes=%lu-%lu", 
                m_range_start, m_range_end);
        //增加Range头域，避免大文件时将整个文件下载
        http_params["Range"] = range_head;

        string rspbuf;
        m_retcode = HttpSender::SendGetRequest(&rspbuf, m_url, http_params, user_params);
        if (taskSuccess()) {
            //当实际长度小于请求的数据长度时httpcode为206
            VLOG(5) << "FileDownload: url: " << m_url << ", range_header:" << range_head \
                <<", http_code: " << m_retcode << ", rsp_size:" << rspbuf.length();
        } else {
            //当实际长度小于请求的数据长度时httpcode为206
            LOG(ERROR) << "FileDownload: url: " << m_url << ", range_header:" << range_head \
                <<", http_code: " << m_retcode;
        }

        size_t buf_max_size = m_range_end - m_range_start + 1;
        size_t len = MIN(rspbuf.length(), buf_max_size);
        memcpy(m_downbuf, rspbuf.c_str(), len);
        m_download_size = len;
    }

    void FileDownloadTask::setRangeStart(uint64_t rangeStart)
    {
        m_range_start = rangeStart;
    }

    void FileDownloadTask::setRangeEnd(uint64_t rangeEnd)
    {
        m_range_end = rangeEnd;
    }

    void FileDownloadTask::setDownBuf(char* downbuf)
    {
        m_downbuf = downbuf;
    }

    bool FileDownloadTask::taskSuccess() {
        return (m_retcode == 200 || m_retcode == 206);
    }

    uint64_t FileDownloadTask::getDownloadSize() {
        return m_download_size;
    }

}

