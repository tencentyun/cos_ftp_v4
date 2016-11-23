#ifndef FILE_OP_H
#define FILE_OP_H
#include <string>
#include "op/BaseTask.h"
#include "op/BaseOp.h"
#include "util/FileUtil.h"
#include "util/HttpUtil.h"
#include "util/CodecUtil.h"
#include "util/HttpSender.h"
#include "util/StringUtil.h"
#include "op/FileUploadTask.h"
#include "op/FileDownloadTask.h"
#include "request/FileUploadReq.h"
#include "request/FileDownloadReq.h"
#include "request/FileUpdateReq.h"
#include "request/FileStatReq.h"
#include "request/FileDeleteReq.h"
#include "request/FileMoveReq.h"
#include "request/CosResult.h"
#include "CosParams.h"
#include "CosDefines.h"
#include "CosConfig.h"
#include "CosSysConfig.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using std::string;
namespace qcloud_cos{

class FileOp;

class UploadCallBackArgs{
public:
    UploadCallBackArgs(){}
    UploadCallBackArgs(const FileUploadReq req, string rsp) : m_req(req){
        m_rsp = rsp;
    }

    FileUploadReq m_req;
    string    m_rsp;
};

class DownloadCallBackArgs {
public:
    DownloadCallBackArgs(){}
    DownloadCallBackArgs(const FileDownloadReq req, char* buffer, size_t buf_len, size_t data_size, int code, string msg) : m_req(req) {
        m_buffer = buffer;
        m_buffer_len = buf_len;
        m_data_size = data_size;
        m_code = code;
        m_message = msg;
    }

    FileDownloadReq m_req; //客户端传入的请求消息
    char*  m_buffer;    //客户端传入的保存数据的buffer
    size_t m_buffer_len;//客户端传入的预读取数据的大小
    size_t m_data_size; //实际读取的数据大小
    int    m_code;      //异步下载返回码,0:成功,非0:失败
    string m_message;   //下载失败的描述
};

typedef void (*UploadCallback)(UploadCallBackArgs* arg);
typedef void (*DownloadCallback)(DownloadCallBackArgs* arg);


class Asyn_Arg {
public:
    Asyn_Arg(FileOp* op) : m_op(op){}
    virtual ~Asyn_Arg(){};
    FileOp* m_op;
};

class Upload_Asyn_Arg : public Asyn_Arg {
public:
    Upload_Asyn_Arg(FileOp* pFileOp,const FileUploadReq& req, UploadCallback cb) : Asyn_Arg(pFileOp) , m_req(req)
    {
        m_callback = cb;
    }
    virtual ~Upload_Asyn_Arg(){}
    FileUploadReq  m_req;
    UploadCallback m_callback;
};

class Download_Asyn_Arg : public Asyn_Arg {
public:
    Download_Asyn_Arg(FileOp* pFileOp, const FileDownloadReq& req, char* buffer, size_t bufLen, DownloadCallback cb) : Asyn_Arg(pFileOp),m_req(req) {
        m_callback = cb;
        m_buffer = buffer;
        m_bufLen = bufLen;
    }

    virtual ~Download_Asyn_Arg(){}

    char*  m_buffer;
    size_t m_bufLen;
    FileDownloadReq  m_req;
    DownloadCallback  m_callback;
};


extern "C" {
void  FileDownload_Asyn_Thread(void * arg);
void  FileUpload_Asyn_Thread(void * arg);
}

class FileOp : public BaseOp {

public:
    FileOp(CosConfig& config) : BaseOp(config) {};
    ~FileOp(){};
    int    FileDownload(FileDownloadReq& req, char* buffer, size_t bufLen, uint64_t offset, int* ret_code);
    bool   FileDownloadAsyn(FileOp& op,const FileDownloadReq& req, char* buffer, size_t bufLen, DownloadCallback callback);
    string FileUpload(FileUploadReq& req);
    bool   FileUploadAsyn(FileOp& op,const FileUploadReq& req, UploadCallback callback);
    string FileUpdate(FileUpdateReq& req);
    string FileStat(FileStatReq& req);
    string FileDelete(FileDeleteReq& req);
    string FileMove(FileMoveReq& req);
    string FileUploadSliceList(FileUploadReq& req);
    string FileUploadSingle(FileUploadReq& req);
    string FileUploadSlice(FileUploadReq& req);

private:
    typedef boost::packaged_task<void> task_t;
    typedef boost::shared_ptr<task_t> ptask_t;
    void push_job(boost::asio::io_service& io_service, std::vector<boost::shared_future<void> >& pending_data, BaseTask *pBastTask);
    void build_io_service(boost::asio::io_service &io_service, boost::thread_group &threads, uint16_t thread_count=0);
    FileOp();
    string FileUploadSliceFinish(FileUploadReq& req, string& dataRspJson);
    string FileUploadSliceData(FileUploadReq& req, string& initRsp);
    string FileUploadSliceInit(FileUploadReq& req);
};

}


#endif
