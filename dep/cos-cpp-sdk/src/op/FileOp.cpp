#include <string.h>
#include <stdint.h>
#include <map>
#include <string>
#include "op/FileOp.h"
#include "op/BaseTask.h"
#include "Auth.h"
#include <boost/bind.hpp>
#include "glog/logging.h"

using std::string;
using std::map;
namespace qcloud_cos{

    void FileOp::build_io_service(boost::asio::io_service &io_service, boost::thread_group &threads, uint16_t thread_count) {
        if (thread_count == 0) {
            thread_count = boost::thread::hardware_concurrency();
        }
        for (uint16_t i = 0; i < thread_count ; ++i)
        {
            threads.create_thread(boost::bind(&boost::asio::io_service::run,
                        &io_service));
        }
    }

    void FileOp::push_job(boost::asio::io_service &io_service, std::vector<boost::shared_future<void> >& pending_data, BaseTask *pBaseTask) {
        ptask_t task = boost::make_shared<task_t>(boost::bind(&BaseTask::run, pBaseTask));
        boost::shared_future<void> fut(task->get_future());
        pending_data.push_back(fut);
        io_service.post(boost::bind(&task_t::operator(), task));
    }

    //文件下载
    int FileOp::FileDownload(FileDownloadReq& req, char* buffer, size_t bufLen, uint64_t offset, int* ret_code)
    {
        LOG(INFO) << "download req: " << req.toJsonString();
        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "FileDownload req ileagl, " << req.toJsonString();
            *ret_code = -1;
            return -1;
        }

        if (!buffer)
        {
            LOG(ERROR) << "FileDownload buffer is null, error";
            *ret_code = -2;
            return -2;
        }

        //多次签名
        uint64_t expired = FileUtil::GetExpiredTime();
        string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                expired, req.getBucket());
        //构造URL
        string url = HttpUtil::GetEncodedDownloadCosUrl(getAppid(),req.getBucket(),req.getFilePath(),
                CosSysConfig::getDownloadDomain(),sign);        
        LOG(INFO) << "download url: " << url;

        // 保存执行的task 指针
        std::vector<FileDownloadTask*> downLoadTaskVec;

        // 构造boost线程池
        boost::asio::io_service io_service;
        boost::thread_group threads;
        boost::asio::io_service::work work(io_service);
        build_io_service(io_service, threads);
        std::vector<boost::shared_future<void> > pending_data; // vector of futures

        uint64_t rangeSize = DEFAULT_DOWN_RANGE_SIZE;
        uint64_t offset_end = offset + bufLen;
        uint64_t total_download_len = 0;
        uint64_t offset_orgin = offset;
        bool task_fail_flag = false;
        while(offset < offset_end)
        {
            FileDownloadTask *ptask = new FileDownloadTask(getAppid(), req.getBucket(), url);
            downLoadTaskVec.push_back(ptask);
            ptask->setDownBuf(buffer + (offset - offset_orgin));
            ptask->setRangeStart(offset);
            uint64_t temp_rangeEnd = offset + rangeSize - 1;
            if (temp_rangeEnd >= offset_end) {
                ptask->setRangeEnd(offset_end - 1);
                offset = offset_end;
            } else {
                ptask->setRangeEnd(temp_rangeEnd);
                offset = temp_rangeEnd + 1;
            }
            push_job(io_service, pending_data, ptask);
        }

        boost::wait_for_all(pending_data.begin(), pending_data.end());
        pending_data.clear();
        io_service.stop();
        threads.join_all();

        size_t task_count = downLoadTaskVec.size();
        for (size_t task_index = 0; task_index < task_count; ++task_index) {
            FileDownloadTask *ptask = downLoadTaskVec[task_index];
            if (ptask->taskSuccess() == false) {
                task_fail_flag = true;
            } else {
                total_download_len += ptask->getDownloadSize();
            }
            delete ptask;
            ptask = NULL;
        }


        if (task_fail_flag) {
            *ret_code = -3;
            LOG(ERROR) << " download failed, url: " << url;
            return -3;
        } else {
            *ret_code = 0;
            LOG(INFO) << "download success, url:" << url \
                << ", return_len:" << total_download_len;
            return total_download_len;
        }

    }

    //文件上传
    string FileOp::FileUpload(FileUploadReq& req)
    {
        string uploadFileRet;
        if (req.getFileSize() < SINGLE_FILE_SIZE) {
            uploadFileRet = FileUploadSingle(req);
        } else {
            uploadFileRet = FileUploadSlice(req);
        }
        Json::Value uploadFileJson = StringUtil::StringToJson(uploadFileRet);
        if (uploadFileJson["code"] == 0) {
            return uploadFileRet;
        }
        // 上传失败, 则删除残损文件
        string dest_path = req.getDstPath();
        string bucket = req.getBucket();
        LOG(ERROR) << "upload file failed, delete file: " << dest_path;
        FileDeleteReq del_req(bucket, dest_path);
        FileDelete(del_req);
    }

    //单文件上传
    string FileOp::FileUploadSingle(FileUploadReq& req)
    {
        LOG(INFO) << "FileUploadSingle req: " << req.toJsonString();
        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "FileUploadSingle illegal req: " << req.toJsonString();
            return ret.toJsonString();
        }

        //读取文件内容
        const unsigned char * pbuf = NULL;
        unsigned int len = 0;
        string fileContent;
        if (!req.isBufferUpload())
        {
            fileContent = FileUtil::getFileContent(req.getSrcPath());
            pbuf = (unsigned char *)fileContent.c_str();
            len = fileContent.length();
        } else {
            pbuf = (unsigned char *)req.getBuffer();
            len  = req.getBufferLen();
        }

        //计算sha值
        string sha1Digest = req.getSha();
        //多次签名
        uint64_t expired = FileUtil::GetExpiredTime();
        string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),
                expired, req.getBucket());
        map<string,string> user_headers;
        user_headers[HTTP_HEADER_AUTHORIZATION] = sign;
        map<string,string> user_params;
        user_params[PARA_OP] = OP_UPLOAD;
        user_params[PARA_INSERT_ONLY] = StringUtil::IntToString(req.getInsertOnly());
        if (CosSysConfig::isTakeSha()){
            user_params[PARA_SHA] = sha1Digest;
        }
        if (!req.getBizAttr().empty()){
            user_params[PARA_BIZ_ATTR] = req.getBizAttr();
        }

        //构造URL
        string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

        //发送上传消息
        string uploadSingleFileRet = HttpSender::SendSingleFilePostRequest(url,user_headers,user_params, pbuf, len);
        printOpResult("FileUploadSingle", uploadSingleFileRet);
        return uploadSingleFileRet;
    }

    //分片文件上传
    string FileOp::FileUploadSlice(FileUploadReq& req)
    {
        LOG(INFO) << "upload slice req: " << req.toJsonString();
        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "upload slice req ileagl, req:" << req.toJsonString();
            return ret.toJsonString();
        }

        //暂时不支持从内存中进行分片上传
        if (req.isBufferUpload())
        {
            ret.setCode(PARA_ERROR_CODE);
            ret.setMessage("not support buffer upload larger than 8M");           
            return ret.toJsonString();
        }

        string initRsp = FileUploadSliceInit(req);
        Json::Value initRspJson = StringUtil::StringToJson(initRsp);

        if (initRspJson["code"] != 0)
        {
            LOG(ERROR) << "slice init failed,  req: " << req.toJsonString() << ", resp: " << initRsp;
            return initRsp;
        }

        LOG(INFO) << "slice init resp: " << initRsp;

        // 表示秒传，则结束
        if (initRspJson["data"].isMember("access_url")) {
            return initRsp;
        }

        string dataRsp = FileUploadSliceData(req, initRsp);
        LOG(INFO) << "slice data resp: " << dataRsp;
        Json::Value dataRspJson = StringUtil::StringToJson(dataRsp);
        if (dataRspJson["code"] != 0)
        {
            LOG(ERROR) << "slice data failed, req: " << req.toJsonString() << ", resp: " << dataRsp;
            return dataRsp;
        }

        string finishRsp = FileUploadSliceFinish(req, dataRsp);
        Json::Value finishRspJson = StringUtil::StringToJson(finishRsp);
        printOpResult("FileUploadSlice", finishRsp);
        return finishRsp;
    }

    string FileOp::FileUploadSliceInit(FileUploadReq& req)
    {
        //计算sha值
        string uploadparts;
        string sha1;
        if(CosSysConfig::isTakeSha()) {
            //设置,后面在data和finish中调用
            CodecUtil::conv_file_to_upload_parts(req.getSrcPath(),req.getSliceSize(),uploadparts, sha1);
            req.setSha(sha1);
        }

        //多次签名
        uint64_t expired = FileUtil::GetExpiredTime();
        string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired,req.getBucket());
        map<string,string> user_headers;
        user_headers[HTTP_HEADER_AUTHORIZATION] = sign;

        map<string,string> user_params;
        user_params[PARA_OP] = OP_UPLOAD_SLICE_INIT;
        user_params[PARA_INSERT_ONLY] =StringUtil::IntToString(req.getInsertOnly());
        user_params[PARA_SLICE_SIZE] = StringUtil::IntToString(req.getSliceSize());
        user_params[PARA_FILE_SIZE] = StringUtil::Uint64ToString(req.getFileSize());
        if (CosSysConfig::isTakeSha()){
            user_params[PARA_UPLOAD_PARTS] = uploadparts;
            user_params[PARA_SHA] = sha1;
        }
        if (!req.getBizAttr().empty()){
            user_params[PARA_BIZ_ATTR] = req.getBizAttr();
        }

        //构造URL
        string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

        //发送上传消息
        return HttpSender::SendSingleFilePostRequest(url,user_headers,
                user_params, NULL, 0);
    }

    string FileOp::FileUploadSliceData(FileUploadReq& req, string& initRsp)
    {
        string bucket = req.getBucket();
        Json::Value initRspJson = StringUtil::StringToJson(initRsp);
        if (!initRspJson.isMember("data")){
            LOG(ERROR) << "FileUploadSliceData no data: " << initRsp;
            return CosResult(PARA_ERROR_CODE, PARA_ERROR_DESC).toJsonString();
        }
        int slice_size = initRspJson["data"]["slice_size"].asInt();
        string sessionid = initRspJson["data"]["session"].asString();   
        int serial_upload = 0;
        if (initRspJson["data"].isMember("serial_upload")) {
            serial_upload = initRspJson["data"]["serial_upload"].asInt();
        }
        uint64_t expired = FileUtil::GetExpiredTime();
        string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, bucket);

        //构造URL
        string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

        std::ifstream fin(req.getSrcPath().c_str(), std::ios::in | std::ios::binary);
        if (!fin.is_open()){
            LOG(ERROR) << "FileUploadSliceData: file open fail: " << req.getSrcPath();
            return CosResult(LOCAL_FILE_NOT_EXIST_CODE,LOCAL_FILE_NOT_EXIST_DESC).toJsonString();
        }

        uint64_t filesize = FileUtil::getFileLen(req.getSrcPath());
        uint64_t offset = 0;

        uint16_t pool_size = THREAD_POOL_SIZE_UPLOAD_SLICE;
        if (serial_upload == 1) {
            pool_size = 1;
        } else {
#ifdef USE_CURL_MULTI        
            std::map<string, string> user_headers;
            user_headers[HTTP_HEADER_AUTHORIZATION] = sign;
            std::map<string, string> user_params;
            user_params[PARA_OP] = OP_UPLOAD_SLICE_DATA;
            user_params[PARA_INSERT_ONLY] = StringUtil::IntToString(req.getInsertOnly());
            user_params[PARA_SLICE_SIZE] = StringUtil::IntToString(req.getSliceSize());
            return HttpSender::SendFileParall(url, user_headers, user_params, req.getSrcPath(), 0, slice_size);
#endif
        }

        bool task_fail_flag = false;

        unsigned char ** fileContentBuf = new unsigned char *[pool_size];
        for(uint16_t i = 0; i < pool_size; ++i){
            fileContentBuf[i] = new unsigned char[slice_size];
        }

        FileUploadTask **pptaskArr = new FileUploadTask*[pool_size];
        for (uint16_t i = 0; i < pool_size; ++i)
        {
            pptaskArr[i] = new FileUploadTask(url, sessionid, sign, req.getSha());
        }

        // 构造boost线程池
        boost::asio::io_service io_service;
        boost::thread_group threads;
        boost::asio::io_service::work work(io_service);
        build_io_service(io_service, threads, pool_size);
        std::vector<boost::shared_future<void> > pending_data; // vector of futures

        LOG(INFO) << "upload data, url=" << url << ", poolsize=" << pool_size << ", slice_size="<< slice_size << ", serial_upload=" << serial_upload;
        string taskResp;//记录线程中返回的错误响应消息
        while(offset < filesize)
        {
            uint16_t task_index = 0;
            for (; task_index < pool_size; ++task_index) {

                //unsigned char * fileContentBuf = new unsigned char[slice_size];
                fin.read((char *)fileContentBuf[task_index],slice_size);
                size_t read_len = fin.gcount();
                if (read_len == 0 && fin.eof()) {
                    VLOG(5) << "read over, url: " << url << ", task_index: " << task_index;
                    break;
                }

                VLOG(5) << "upload slice, url=" << url << ", task_index=" << task_index << ", filesize=" << filesize << ", offset=" << offset << ", read_len=" <<read_len;

                FileUploadTask *ptask = pptaskArr[task_index];
                ptask->setOffset(offset);
                ptask->setBufdata(fileContentBuf[task_index], read_len);

                push_job(io_service, pending_data, ptask);

                offset += read_len;
            }

            boost::wait_for_all(pending_data.begin(), pending_data.end());
            pending_data.clear();
            uint16_t max_task_num = task_index;
            for (task_index = 0; task_index < max_task_num; ++task_index) {
                FileUploadTask *ptask = pptaskArr[task_index];
                if (ptask->taskSuccess() == false) {
                    taskResp = ptask->getTaskResp();
                    task_fail_flag = true;
                    break;
                }
            }

            if (task_fail_flag) {
                break;
            }

            if (taskResp.empty()) {
                taskResp = (pptaskArr[max_task_num - 1])->getTaskResp();
            }
        }

        // close file, clear thread pool, release memory
        fin.close();
        io_service.stop();
        threads.join_all();
        for (uint16_t i = 0; i < pool_size; ++i)
        {
            delete pptaskArr[i];
        }
        delete [] pptaskArr;

        for (uint16_t i = 0; i < pool_size; ++i) {
            delete [] fileContentBuf[i];
        }
        delete [] fileContentBuf;

        return taskResp;
    }

    string FileOp::FileUploadSliceFinish(FileUploadReq& req, string& dataRspJson)
    {
        Json::Value rspJson = StringUtil::StringToJson(dataRspJson);
        string sessionid = rspJson["data"]["session"].asString();

        //多次签名
        uint64_t expired = FileUtil::GetExpiredTime();
        string sign = Auth::AppSignMuti(getAppid(),getSecretID(),getSecretKey(),expired, req.getBucket());
        map<string,string> user_headers;
        user_headers[HTTP_HEADER_AUTHORIZATION] = sign;

        map<string,string> user_params;
        user_params[PARA_OP] = OP_UPLOAD_SLICE_FINISH;
        user_params[PARA_SESSION] = sessionid;
        user_params[PARA_FILE_SIZE] = StringUtil::Uint64ToString(req.getFileSize());
        if (CosSysConfig::isTakeSha()){
            user_params[PARA_SHA] = req.getSha();
        }

        //构造URL
        string url = HttpUtil::GetEncodedCosUrl(getAppid(),req.getBucket(),req.getDstPath());

        //发送上传消息
        string finishRsp = HttpSender::SendSingleFilePostRequest(url,user_headers,
                user_params, NULL, 0);
        return finishRsp;
    }

    string FileOp::FileUpdate(FileUpdateReq& req)
    {
        LOG(INFO) << "FileUpdate req: " << req.toJsonString();
        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "FileUpdate req ileagl, req:" << req.toJsonString();
            return ret.toJsonString();
        }

        string bucket = req.getBucket();
        string formatPath = req.getFormatPath();
        string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);
        string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                formatPath,bucket);
        LOG(INFO) << "update file, url:" << fullUrl << ", sign:" << sign;

        std::map<string, string> http_headers;
        http_headers[HTTP_HEADER_AUTHORIZATION] = sign;

        Json::Value body;
        body[PARA_OP] = OP_UPDATE;
        if (req.isUpdateForBid()){
            body[PARA_FORBID] = req.getForbid();    
        }
        if (req.isUpdateAuthority()){
            body[PARA_AUTHORITY] = req.getAuthority();
        }
        if (req.isUpdateBizAttr()){
            body[PARA_BIZ_ATTR] = req.getBizAttr();
        }
        if (req.isUpdateCustomHeader()){
            map<string,string> map_custom_headers = req.getCustomHeaders();
            map<string,string>::const_iterator iter = map_custom_headers.begin();
            for (;iter != map_custom_headers.end(); iter++)
            {
                body[PARA_CUSTOM_HEADERS][iter->first] = iter->second;
            }
        }
        string bodyJson = StringUtil::JsonToString(body);    
        string updateFileRet = HttpSender::SendJsonBodyPostRequest(fullUrl, bodyJson, http_headers);
        printOpResult("FileUpdate", updateFileRet);
        return updateFileRet;
    }

    string FileOp::FileStat(FileStatReq& req)
    {
        LOG(INFO) << "stat file req: " << req.toJsonString();

        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "stat file req ileagl, req: " << req.toJsonString();
            return ret.toJsonString();
        }

        string bucket = req.getBucket();
        string formatPath = req.getFormatPath();

        string statFileRet = statBase(bucket, formatPath);
        printOpResult("FileStat", statFileRet);
        return statFileRet;
    }

    string FileOp::FileDelete(FileDeleteReq& req)
    {
        LOG(INFO) << "delete file req: "<< req.toJsonString();

        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "stat file req ileagl, req: " << req.toJsonString();
            return ret.toJsonString();
        }
        string delFileRet = delBase(req.getBucket(), req.getFilePath());
        printOpResult("FileDelete", delFileRet);
        return delFileRet;
    }

    string FileOp::FileMove(FileMoveReq& req)
    {
        LOG(INFO) << "move file req: "<< req.toJsonString();

        CosResult ret;
        if (!req.isParaValid(ret))
        {
            LOG(ERROR) << "move file req ileagl, req: " << req.toJsonString();
            return ret.toJsonString();
        }

        string bucket = req.getBucket();
        string formatPath = req.getFormatPath();

        string sign = Auth::AppSignOnce(getAppid(),getSecretID(),getSecretKey(),
                formatPath,bucket);
        string fullUrl = HttpUtil::GetEncodedCosUrl(getAppid(),bucket, formatPath);
        LOG(INFO) << "move file, url:" << fullUrl << ", sign:" << sign;

        std::map<string, string> http_headers;
        http_headers[HTTP_HEADER_AUTHORIZATION] = sign;
        std::map<string, string> http_params;
        http_params[PARA_OP] = OP_MOVE;
        http_params[PARA_MOVE_DST_FILEID] = req.getDstPath();
        http_params[PARA_MOVE_OVER_WRITE] = StringUtil::IntToString(req.getOverWrite());
        string moveFileRet = HttpSender::SendJsonPostRequest(fullUrl, http_headers, http_params);
        printOpResult("FileMove", moveFileRet);
        return moveFileRet;
    }

}

