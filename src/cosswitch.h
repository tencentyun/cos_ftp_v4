#ifndef COSSWITCH_H_
#define COSSWITCH_H_

#include <stdint.h>
#include <time.h>

struct vsf_session;

#define TYPE_FILE 1
#define TYPE_DIR  2

#ifdef __cplusplus
extern "C"
{
#endif

#include "str.h"

    enum PERMISSION
    {
        Global_READ = (1 << 0),
        Global_WRITE =(1 << 1)
    };

    struct file_info
    {
        char type;
        struct mystr file_name;
        uint64_t file_size;
        time_t ctime;
        time_t mtime;
    };

    void cos_env_init();
    void cos_env_finit();

    /*
     * return:
     *  0 is success
     * -1 is fail
     * */
    int cos_handle_login(const struct mystr* user, const struct mystr* passwd, int* permission);

    /*
     * return:
     *  the addr of file list
     * */
    void* cos_get_list(struct vsf_session* p_sess, uint32_t timeout, const struct mystr* path,
            int (*filter_func)(const struct mystr*, const struct mystr*, unsigned int*),
            const struct mystr *filter);
    /*
     * param:
     *  `ptr` is get from function cos_get_list
     *
     * return:
     *  0 is ok
     * -1 is empty
     * */
    int cos_list_next(void *ptr, struct file_info *info);
    /*
     * param:
     *  `ptr` is get from function cos_get_list
     * */
    void cos_free_list(void *ptr);

    /*
     * return:
     *  0 is success
     * -1 is error
     * */
    int cos_stat_file(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path, struct file_info *info);
    /*
     * return:
     *  0 is success
     * -1 is error
     * */
    int cos_stat_dir(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path, struct file_info *info);

    /*
     * return:
     *  1 is not empty
     *  0 is empty
     * -1 is error
     * */
    int cos_dir_isempty(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path);
    /*
     * return:
     *  0 is dir
     * -1 not dir or error
     * */
    int cos_isdir(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path);
    /*
     * return:
     *  0 is success
     * -1 is error
     * */
    int cos_mkdir(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path);
    /*
     * return:
     *  1 is not exist
     *  0 is success
     * -1 is error
     * */
    int cos_rmdir(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path);
    /*
     * return:
     *  1 is not exist
     *  0 is success
     * -1 is error
     * */
    int cos_unlink(struct vsf_session* p_sess, uint32_t timeout,
            const struct mystr* path);

    /*
     * return:
     *  0 is success
     * -1 is error
     * */
    int cos_upload_file(struct vsf_session * p_sess,uint32_t timeout,
            const struct mystr* src_path, const struct mystr * path,
            unsigned int slice_size);
    /*
     * return:
     *  >=0 the length download from cos
     *  <0  error
     * */
    int cos_download_file(struct vsf_session * p_sess,uint32_t timeout,
            const struct mystr* path, char buff[], uint32_t bufflen,
            uint64_t offset, uint64_t filesize);

    void cos_sha1_str(const struct mystr * srcStr,struct mystr * dstStr);
    int cos_conv_gbk_2_utf(struct vsf_session * p_sess,struct mystr * ftp_arg_str);

#ifdef __cplusplus
}
#endif

#endif //COSSWITCH_H_
