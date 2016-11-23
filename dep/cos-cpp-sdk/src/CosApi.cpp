#include "CosApi.h"
#include <pthread.h>
#include <string>
#include "curl/curl.h"
#include "glog/logging.h"
using std::string;
namespace qcloud_cos {

int CosAPI::g_init = 0;
int CosAPI::cos_obj_num = 0;
SimpleMutex CosAPI::init_mutex = SimpleMutex();

int CosAPI::FileDownload(FileDownloadReq& request, char* buffer, size_t bufLen, uint64_t offset, int* ret_code)
{
    return fileOp.FileDownload(request, buffer, bufLen, offset, ret_code);
}

string CosAPI::FileUpload(FileUploadReq& request)
{
    return fileOp.FileUpload(request);
}

string CosAPI::FileStat(FileStatReq& request)
{
    return fileOp.FileStat(request);
}

string CosAPI::FileDelete(FileDeleteReq& request)
{
    return fileOp.FileDelete(request);
}
string CosAPI::FileUpdate(FileUpdateReq& request)
{
    return fileOp.FileUpdate(request);
}
string CosAPI::FolderCreate(FolderCreateReq& request)
{
    return folderOp.FolderCreate(request);
}
string CosAPI::FolderStat(FolderStatReq& request)
{
    return folderOp.FolderStat(request);
}
string CosAPI::FolderDelete(FolderDeleteReq& request)
{
    return folderOp.FolderDelete(request);
}
string CosAPI::FolderUpdate(FolderUpdateReq& request)
{
    return folderOp.FolderUpdate(request);
}
string CosAPI::FolderList(FolderListReq& request)
{
    return folderOp.FolderList(request);
}

CosAPI::CosAPI(CosConfig& config):fileOp(config),folderOp(config)
{
    COS_Init();
}

CosAPI::~CosAPI()
{
    COS_UInit();
}

int CosAPI::COS_Init() {
    SimpleMutexLocker locker(&init_mutex);   
    ++cos_obj_num;
    if (!g_init) {
        g_init = true;

        FLAGS_stderrthreshold = 4;
        FLAGS_logbuflevel = -1;
        google::InitGoogleLogging("ftp_sdk");
    }
    return 0;
}

void CosAPI::COS_UInit() {
    SimpleMutexLocker locker(&init_mutex);
    --cos_obj_num;
    if (g_init && cos_obj_num == 0) {
        g_init = false;
    }
}

}
