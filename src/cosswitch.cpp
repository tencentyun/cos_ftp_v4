#include "cosswitch.h"
#include "CosApi.h"
#include <list>
#include <stdlib.h>
#include <openssl/sha.h>
#include <unistd.h>
extern "C"
{
#include "session.h"
#include "logging.h"
#include "sysstr.h"
#include "ftpcmdio.h"
#include "tunables.h"
}
#include "encode.h"
#include <string.h>

typedef std::list<struct file_info> file_list;
static qcloud_cos::CosAPI *s_cosapi;

static bool has_init = false;
void cos_env_init()
{
	if (has_init) return;

    qcloud_cos::CosSysConfig::setExpiredTime(tunable_cos_sign_expire_s);
    qcloud_cos::CosSysConfig::setTimeoutInms(tunable_cos_conn_timeout_ms);
    qcloud_cos::CosSysConfig::setGlobalTimeoutInms(tunable_cos_req_timeout_ms);
    qcloud_cos::CosSysConfig::setIsTakeSha(tunable_cos_take_sha==1);
    qcloud_cos::CosSysConfig::setSliceSize(tunable_cos_upload_slice_size);
    qcloud_cos::CosSysConfig::setCosRegion(tunable_cos_region);

    if (strcasecmp(tunable_cos_download_domain, "cdn") == 0) {
        qcloud_cos::CosSysConfig::setDownloadDomain(true);
    }

    qcloud_cos::CosConfig config(atol(tunable_cos_appid), tunable_cos_secretid, tunable_cos_secretkey);
    s_cosapi = new qcloud_cos::CosAPI(config);

    has_init = true;
}

void cos_env_finit()
{
	if (! has_init) return;

    delete s_cosapi;
    s_cosapi = NULL;

	has_init = false;
}

int cos_handle_login(const struct mystr* user, const struct mystr* passwd, int* permission)
{
    int result = -1;
    if (!tunable_login_users) return result;

    char *save_ptr1 = 0, *save_ptr2 = 0;
    char *login_users = strdup(tunable_login_users);
    for(char *tp = strtok_r(login_users, ";", &save_ptr1);
            tp != NULL;
            tp = strtok_r(NULL, ";", &save_ptr1))
    {
        char *myuser = strtok_r(tp, ":", &save_ptr2);
        if (!myuser) continue;
        char *mypasswd = strtok_r(NULL, ":", &save_ptr2);
        if (!mypasswd) continue;
        char *mypermission = strtok_r(NULL, ":", &save_ptr2);
        if (!mypermission) continue;

        if (strcmp(myuser, str_getbuf(user)) == 0
                && strcmp(mypasswd, str_getbuf(passwd)) == 0)
        {
            *permission = 0;
            while ((*mypermission) != '\0')
            {
                if (*mypermission == 'R' || *mypermission == 'r')
                    *permission |= Global_READ;
                else if (*mypermission == 'W' || *mypermission == 'w')
                    *permission |= Global_WRITE;

                ++mypermission;
            }
            result = 0;
            break;
        }
    }
    free(login_users);

    return result;
}

/* start with '/', end with `p_list_path` */
void get_cos_path_from_disk_path(struct mystr* p_cos_path,
        const struct mystr* p_list_path, struct vsf_session* p_sess)
{
	str_empty(p_cos_path);
	if (str_isempty(p_list_path) || str_get_char_at(p_list_path, 0) != '/')
	{
		str_session_getcwd(p_sess, p_cos_path);
		str_append_char(p_cos_path, '/');
	}
	str_append_str(p_cos_path, p_list_path);
}

void* cos_get_list(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path,
      int (*filter_func)(const struct mystr*, const struct mystr*, unsigned int*),
      const struct mystr *filter)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);
    if (str_get_char_at(&s_cos_path, str_getlen(&s_cos_path)-1) != '/')
        str_append_char(&s_cos_path, '/');

	std::string context = "";
	file_list *flist = new file_list;
	int try_times = 2;
	while (try_times > 0)
    {
        std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
        qcloud_cos::FolderListReq flreq(bucket, cos_path, 1000, true, context);
        std::string result = s_cosapi->FolderList(flreq);

        Json::Value root;
        Json::Reader reader;

        if (reader.parse(result, root)
                && root.isMember("code") && root["code"].isInt() && root["code"].asInt() == 0
                && root.isMember("data") && root["data"].isObject()
                && root["data"].isMember("infos") && root["data"]["infos"].isArray()
                && root["data"].isMember("listover") && root["data"]["listover"].isBool()
                && root["data"].isMember("context") && root["data"]["context"].isString()
           )
        {
            Json::Value &infos = root["data"]["infos"];
            for (Json::Value::iterator iter = infos.begin(); iter != infos.end(); ++iter)
            {
                unsigned int filter_iter = 0;
                struct file_info info = {0, INIT_MYSTR, 0, 0, 0};
                str_alloc_text(&info.file_name,
                        (((*iter).isMember("name")&&(*iter)["name"].isString())
                         ? (*iter)["name"].asCString() : ""));
                if (filter_func != NULL && !filter_func(&info.file_name, filter, &filter_iter))
                    continue;
                
                info.type = ((*iter).isMember("sha") ? TYPE_FILE : TYPE_DIR);
                info.file_size = (((*iter).isMember("filelen")&&(*iter)["filelen"].isInt64())
                        ? (*iter)["filelen"].asInt64() : 0);
                info.ctime = (((*iter).isMember("ctime")&&(*iter)["ctime"].isInt())
                        ? (*iter)["ctime"].asInt() : 0);
                info.mtime = (((*iter).isMember("mtime")&&(*iter)["mtime"].isInt())
                        ? (*iter)["mtime"].asInt() : 0);
                flist->push_back(info);

                if (tunable_list_max_count > 0 && flist->size() >= tunable_list_max_count)
                {
                    return (void*)flist;
                }
            }

            if (root["data"]["listover"].asBool()) break;

            context = root["data"]["context"].asString();
        } else
        {
            struct mystr respmsg = INIT_MYSTR;

            str_alloc_text(&respmsg, result.c_str());
            vsf_log_line(p_sess, kVSFLogEntryDebug, &respmsg);
			str_free(&respmsg);

			try_times --;
			continue;
		}
	}
	return (void*)flist;
}

int cos_list_next(void *ptr, struct file_info *info)
{
	file_list *flist = (file_list *)ptr;
	if (flist == NULL || flist->empty()) return -1;

	struct file_info &t_info = flist->front();

	str_copy(&info->file_name, &t_info.file_name);
	info->type = t_info.type;
	info->file_size = t_info.file_size;
	info->ctime = t_info.ctime;
	info->mtime = t_info.mtime;

	str_free(&t_info.file_name);
	flist->pop_front();
	return 0;
}

void cos_free_list(void *ptr)
{
	file_list *flist = (file_list *)ptr;
	if (flist && !flist->empty())
	{
		while (!flist->empty())
		{
			struct file_info &t_info = flist->front();

			str_free(&t_info.file_name);
			flist->pop_front();
		}
	}
	delete flist;
}

int cos_stat_file(struct vsf_session* p_sess, uint32_t timeout,
		const struct mystr* path, struct file_info *info)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FileStatReq fsreq(bucket, cos_path);
    std::string result = s_cosapi->FileStat(fsreq);

    Json::Value root;
    Json::Reader reader;

    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt() && root["code"].asInt() == 0
            && root.isMember("data") && root["data"].isObject()
       )
    {
        if (info != NULL)
        {
            Json::Value &data = root["data"];

            info->type = TYPE_FILE;
            str_copy(&info->file_name, path);
            info->file_size = ((data.isMember("filelen")&&data["filelen"].isInt64())
                    ? data["filelen"].asInt64() : 0);
            info->ctime = ((data.isMember("ctime")&&data["ctime"].isInt()) ? data["ctime"].asInt() : 0);
            info->mtime = ((data.isMember("mtime")&&data["mtime"].isInt()) ? data["mtime"].asInt() : 0);
        }

        return 0;
    } else
	{
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
		vsf_log_line(p_sess, kVSFLogEntryDebug, &respmsg);
		str_free(&respmsg);

		return -1;
	}
}
int cos_stat_dir(struct vsf_session* p_sess, uint32_t timeout,
		const struct mystr* path, struct file_info *info)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);
    if (str_get_char_at(&s_cos_path, str_getlen(&s_cos_path)-1) != '/')
        str_append_char(&s_cos_path, '/');

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FolderStatReq fsreq(bucket, cos_path);
    std::string result = s_cosapi->FolderStat(fsreq);

    Json::Value root;
    Json::Reader reader;

    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt() && root["code"].asInt() == 0
            && root.isMember("data") && root["data"].isObject()
       )
    {
        if (info != NULL)
        {
            Json::Value &data = root["data"];

            info->type = TYPE_DIR;
            str_copy(&info->file_name, path);
            info->file_size = 0;
            info->ctime = ((data.isMember("ctime")&&data["ctime"].isInt()) ? data["ctime"].asInt() : 0);
            info->mtime = ((data.isMember("mtime")&&data["mtime"].isInt()) ? data["mtime"].asInt() : 0);
        }

        return 0;
    } else
	{
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
		vsf_log_line(p_sess, kVSFLogEntryDebug, &respmsg);
		str_free(&respmsg);

		return -1;
	}
}

int cos_dir_isempty(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);
    if (str_get_char_at(&s_cos_path, str_getlen(&s_cos_path)-1) != '/')
        str_append_char(&s_cos_path, '/');

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FolderListReq flreq(bucket, cos_path, 3, true, "");
    std::string result = s_cosapi->FolderList(flreq);

    Json::Value root;
    Json::Reader reader;

    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt() && root["code"].asInt() == 0
            && root.isMember("data") && root["data"].isObject()
            && root["data"].isMember("infos") && root["data"]["infos"].isArray()
            && root["data"].isMember("listover") && root["data"]["listover"].isBool()
       )
    {
        if (root["data"]["infos"].empty() && root["data"]["listover"].asBool())
            return 0;
        return 1;
    } else
    {
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
        vsf_log_line(p_sess, kVSFLogEntryDebug, &respmsg);
        str_free(&respmsg);

        return -1;
    }
}

int cos_isdir(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path)
{
    //dir is exist
    if (cos_stat_dir(p_sess, timeout, path, NULL) == 0)
        return 0;

#if 1 //for not exist dir
    //dir is not exist, but there are files in it.
    if (cos_dir_isempty(p_sess, timeout, path) == 1)
        return 0;
#endif

    return -1;
}

int cos_mkdir(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);
    if (str_get_char_at(&s_cos_path, str_getlen(&s_cos_path)-1) != '/')
        str_append_char(&s_cos_path, '/');

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FolderCreateReq fcreq(bucket, cos_path);
    std::string result = s_cosapi->FolderCreate(fcreq);

    Json::Value root;
    Json::Reader reader;

    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt() && root["code"].asInt() == 0
       )
    {
        return 0;
    } else
    {
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
		vsf_log_line(p_sess, kVSFLogEntryMkdir, &respmsg);
		str_free(&respmsg);

		return -1;
    }
}

int cos_rmdir(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);
    if (str_get_char_at(&s_cos_path, str_getlen(&s_cos_path)-1) != '/')
        str_append_char(&s_cos_path, '/');

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FolderDeleteReq fdreq(bucket, cos_path);
    std::string result = s_cosapi->FolderDelete(fdreq);

    Json::Value root;
    Json::Reader reader;

    // success or folder not exist
    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt()
            && (root["code"].asInt() == 0 || root["code"].asInt() == -197)
       )
    {
        if (root["code"].asInt() == 0)
            return 0;
        return 1;
    } else
    {
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
		vsf_log_line(p_sess, kVSFLogEntryRmdir, &respmsg);
		str_free(&respmsg);

		return -1;
    }
}

int cos_unlink(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path)
{
    (void) timeout;
	static struct mystr s_cos_path;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FileDeleteReq fdreq(bucket, cos_path);
    std::string result = s_cosapi->FileDelete(fdreq);

    Json::Value root;
    Json::Reader reader;

    // success or file not exist
    if (reader.parse(result, root)
            && root.isMember("code") && root["code"].isInt()
            && (root["code"].asInt() == 0 || root["code"].asInt() == -197)
       )
    {
        if (root["code"].asInt() == 0)
            return 0;
        return 1;
    } else
    {
        struct mystr respmsg = INIT_MYSTR;

        str_alloc_text(&respmsg, result.c_str());
		vsf_log_line(p_sess, kVSFLogEntryDelete, &respmsg);
		str_free(&respmsg);

		return -1;
    }
}

void print_oneonezero(void *data, Json::Value &ret)
{
	return ;
	(void)ret;
	vsf_session *p_sess = (vsf_session *)data;
	if (p_sess)
		vsf_cmdio_write(p_sess, 110, "MARK yyyy=mmmm");
}

int cos_upload_file(struct vsf_session* p_sess, uint32_t timeout,
		const struct mystr* src_path,
		const struct mystr* path, unsigned int slice_size)
{
    (void) timeout;
    (void) slice_size;
	static struct mystr s_cos_path = INIT_MYSTR;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);

    std::string bucket(tunable_cos_bucket),
        cos_path(str_getbuf(&s_cos_path)),
        file_path(str_getbuf(src_path));
    qcloud_cos::FileUploadReq fureq(bucket, file_path, cos_path);
    fureq.setInsertOnly(0);

	int trytime = 2;
/*
    while(trytime == 2) {
        sleep(1);
    }
*/
	do
	{
        std::string result = s_cosapi->FileUpload(fureq);
        trytime --;

        Json::Value root;
        Json::Reader reader;

        // success or same file is existing
        // 目前 -4018 还未根据sha值做比较，所以等同于 -177，还不能认为文件相同
        if (reader.parse(result, root)
                //&& root.isMember("code") && root["code"].isInt() && (root["code"].asInt() == 0 || root["code"].asInt() == -4018)
                && root.isMember("code") && root["code"].isInt() && (root["code"].asInt() == 0)
           )
        {
            return 0;
        } else
        {
            struct mystr respmsg = INIT_MYSTR;

            str_alloc_text(&respmsg, result.c_str());
            vsf_log_line(p_sess, kVSFLogEntryUpload, &respmsg);
            str_free(&respmsg);
        }
	} while (trytime > 0);

	return -1;
}

int cos_download_file(struct vsf_session * p_sess,uint32_t timeout,
        const struct mystr* path, char buff[], uint32_t bufflen,
        uint64_t offset, uint64_t filesize)
{
    (void) timeout;
	static struct mystr s_cos_path = INIT_MYSTR;
	get_cos_path_from_disk_path(&s_cos_path, path, p_sess);

    std::string bucket(tunable_cos_bucket), cos_path(str_getbuf(&s_cos_path));
    qcloud_cos::FileDownloadReq fdreq(bucket, cos_path);

	if (bufflen > (filesize - offset)) {
            bufflen = filesize - offset;
	}
	int trytime = 2, recvlen = 0;
	do
	{
        int retcode = 0;
        recvlen = s_cosapi->FileDownload(fdreq, buff, bufflen, offset, &retcode);

        if (recvlen >= 0)
            break;
        else
        {
            struct mystr respmsg = INIT_MYSTR;

            str_alloc_text(&respmsg, "Download retcode:");
            str_append_int(&respmsg, retcode);
            vsf_log_line(p_sess, kVSFLogEntryDownload, &respmsg);
            str_free(&respmsg);
        }
	} while ((--trytime) > 0);

	return recvlen;
}

void cos_sha1_str(
        const struct mystr* srcStr, struct mystr* dstStr) {

    static char hexBytes[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    unsigned char sha1[SHA_DIGEST_LENGTH];

    SHA_CTX ctx;
    SHA1_Init(&ctx);

    SHA1_Update(
                &ctx, str_getbuf(srcStr), str_getlen(srcStr));

    SHA1_Final(sha1, &ctx);

    string shaStr = "";
    for (int i=0; i<SHA_DIGEST_LENGTH; i++) {
        shaStr += hexBytes[sha1[i] >> 4];
        shaStr += hexBytes[sha1[i] & 0x0f];
    }

    str_alloc_text(dstStr, shaStr.c_str());
}

void inner_asc_2_bcd(const unsigned char *asc, unsigned int len, struct mystr *bcd_str) {
	if(!asc || !bcd_str)
		return;

	static char hexBytes[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f'};

	char *bcdStr = (char*)malloc(2 * len + 1);
	int i;
    for (i=0; (unsigned)i < len; i++) {
        bcdStr[2*i] = hexBytes[asc[i] >> 4];
        bcdStr[2*i+1] = hexBytes[asc[i] & 0x0f];
    }

	bcdStr[2*i] = 0;

	str_alloc_text(bcd_str, bcdStr);
	free(bcdStr);
	
}
void  inner_asc_2_bcd(struct mystr *asc_str, struct mystr *bcd_str) {
	if(!asc_str || !bcd_str)
		return;
	
	
	const unsigned char *asc = (const unsigned char *)str_getbuf(asc_str);

	return inner_asc_2_bcd(asc, str_getlen(asc_str), bcd_str);
}

int cos_conv_gbk_2_utf(struct vsf_session* p_sess, struct mystr *ftp_arg_str)
{
	if(0 == str_getlen(ftp_arg_str) || IsUtf8((unsigned char*)str_getbuf(ftp_arg_str))) {
		return 0;
	}

	char temp[str_getlen(ftp_arg_str) * 2 + 1];
	memset(temp, 0, sizeof(temp));
	if ( 0 == CodeConvert("GBK", "UTF-8", str_getbuf(ftp_arg_str), str_getlen(ftp_arg_str), 
		temp, sizeof(temp))) {
		//for debug
		struct mystr tempStr = INIT_MYSTR;
		str_alloc_text(&tempStr, "conv gbk to utf-8, src:");
		str_append_str(&tempStr, ftp_arg_str);
		str_append_text(&tempStr, "  dst:");

		str_append_text(&tempStr, temp);

		//conv it
		str_alloc_text(ftp_arg_str, temp);
		vsf_log_line(p_sess, kVSFLogEntryFTPInput, &tempStr);

		//debug
		struct mystr dstStr = INIT_MYSTR;
		inner_asc_2_bcd((const unsigned char*)temp, strlen(temp), &dstStr);
		vsf_log_line(p_sess, kVSFLogEntryFTPInput, &dstStr);
		str_free(&tempStr);
		str_free(&dstStr);

		return 0;
	} else {
		struct mystr tempStr = INIT_MYSTR;
		str_alloc_text(&tempStr, "conv gbk to utf-8 error:");
		vsf_log_line(p_sess, kVSFLogEntryFTPInput, &tempStr);
		str_free(&tempStr);
		return -1;
	}
}

