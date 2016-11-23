#include "CosApi.h"
#include "request/FileUploadReq.h"
#include "CosConfig.h"
#include "CosParams.h"
#include "CosDefines.h"
#include "CosSysConfig.h"
#include "util/StringUtil.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <time.h>

using std::string;
using std::cout;
using std::endl;

uint64_t appid = 10000000;
string secretID = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
string secretKey = "xxxxxxxxxxxxxxxxxxxxxxxxxxx";

using namespace qcloud_cos;

extern "C" {
//异步下载文件的回调函数
void download_callback(DownloadCallBackArgs* arg)
{
    if (!arg){
        return;
    }

    //请求消息是客户端传入的,客户端不用再次释放
    static int i = 0;
    delete arg;
    return;
}

//异步上传文件的回调函数
void upload_callback(UploadCallBackArgs* arg)
{
    if (!arg){
        return;
    }

    //请求消息是客户端传入的,客户端不用再次释放
    string rsp = arg->m_rsp;

    delete arg;
    return;
}
}

int main()
{
    string bucket = "mybucket";
    string srcpath = "../boost_1_61_0.zip";
    string srcpath2 = "../testdata/test2";
    string dstpath = "/boost_1_61_0.zip";
    string dstpath2 = "/test2";
    string folder = "/testdata/";
    string folder_biz_attr = "folder attribute";
    string ret = "";
    /*设置cos系统参数*/
    //设置签名超时时长300s
    CosSysConfig::setExpiredTime(300); 
    //设置上传文件中是否携带sha值
    CosSysConfig::setIsTakeSha(false); 
    //设置下载域名
    CosSysConfig::setCosRegion("sh"); 

    //生成CosAPI对象
    CosConfig config(appid,secretID,secretKey);
    CosAPI cos(config);
/*
    //从本地文件中上传文件(异步上传)
    FileUploadReq fileUploadReq1(bucket,srcpath, dstpath);
    //设置允许同名文件覆盖
    fileUploadReq1.setInsertOnly(0);
    cos.FileUploadAsyn(fileUploadReq1, upload_callback);

    FileUploadReq fileUploadReq2(bucket,srcpath2, dstpath2);
    fileUploadReq2.setInsertOnly(0);
    cos.FileUploadAsyn(fileUploadReq2, upload_callback);

    sleep(1);

    char buf[1024];
    FileDownloadReq fileDownloadReq(bucket,dstpath);
    cos.FileDownloadAsyn(fileDownloadReq, buf, 1024, download_callback);
    
    char buf2[1024];
    FileDownloadReq fileDownloadReq2(bucket,dstpath2);
    bool f2 = cos.FileDownloadAsyn(fileDownloadReq2,buf2,1024, download_callback);
    cout << f2 << endl;

    //目录创建
    FolderCreateReq folderCreateReq(bucket,folder,folder_biz_attr);
    ret = cos.FolderCreate(folderCreateReq);
    cout << ret <<endl;

    //目录更新
    folder_biz_attr = "folder new_attribute";
    FolderUpdateReq folderUpdateReq(bucket,folder, folder_biz_attr);
    ret = cos.FolderUpdate(folderUpdateReq);
    cout << ret <<endl;

    //目录查询
    FolderStatReq folderStatReq(bucket,folder);
    ret = cos.FolderStat(folderStatReq);
    cout << ret <<endl;
*/
    //从本地文件中上传文件
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    FileUploadReq fileUploadReq(bucket,srcpath, dstpath);
    ret = cos.FileUpload(fileUploadReq);
    gettimeofday(&end_time, NULL);
    cout << "upload file: " << ret <<endl;
    cout << "upload file time: " << ((end_time.tv_sec - start_time.tv_sec)*1000 + (end_time.tv_usec - start_time.tv_usec) / 1000) << " ms" << endl;
/*
    //从buffer中上传文件(从内存上传暂时只支持小于8M的)
    string testbuf(1024,'A');
    FileUploadReq fileBufUploadReq(bucket,dstpath,testbuf.c_str(), testbuf.length());
    fileBufUploadReq.setInsertOnly(0);
    ret = cos.FileUpload(fileBufUploadReq);
    cout << ret <<endl;

    //文件更新
    map<string, string> custom_header;
    custom_header[PARA_CACHE_CONTROL] = "PARA_CACHE_CONTROL";
    custom_header[PARA_CONTENT_TYPE] = "PARA_CONTENT_TYPE";
    custom_header[PARA_CONTENT_DISPOSITION] = "PARA_CONTENT_DISPOSITION";
    custom_header[PARA_CONTENT_LANGUAGE] = "PARA_CONTENT_LANGUAGE";
    custom_header[PARA_X_COS_META_PREFIX + "abc"] = "PARA_X_COS_META_PREFIX";
    FileUpdateReq fileUpdateReq(bucket,dstpath);
    string file_biz_attr = "file new_attribute";
    string auth = "eInvalid";
    fileUpdateReq.setBizAttr(file_biz_attr);
    fileUpdateReq.setAuthority(auth);
    fileUpdateReq.setForbid(0);
    fileUpdateReq.setCustomHeader(custom_header);
    ret = cos.FileUpdate(fileUpdateReq);
    cout << ret <<endl;    
*/
    //文件查询
    FileStatReq fileStatReq(bucket,dstpath);
    ret = cos.FileStat(fileStatReq);
    cout << "stat file:" << ret <<endl;
/*
    //目录列表
    FolderListReq folderListReq(bucket,folder);
    ret = cos.FolderList(folderListReq);
    cout << ret <<endl;
*/
    //文件删除
    FileDeleteReq fileDeleteReq(bucket,dstpath);
    ret = cos.FileDelete(fileDeleteReq);
    cout << "delete file" << ret <<endl;  
/*
    //目录删除
    FolderDeleteReq folderDeleteReq(bucket,folder);
    ret = cos.FolderDelete(folderDeleteReq);
    cout << ret <<endl;
*/
    return 0;
}

