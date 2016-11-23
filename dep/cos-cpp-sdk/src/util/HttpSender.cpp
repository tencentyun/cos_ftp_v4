// Copyright (c) 2016, Tencent Inc.
// All rights reserved.
//
// Author: Wu Cheng <chengwu@tencent.com>
// Created: 03/08/2016
// Description:
#include "util/HttpSender.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <stdint.h>
#include <openssl/crypto.h>
#include "json/json.h"
#include "CosParams.h"
#include "request/CosResult.h"
#include "CosSysConfig.h"
#include "CosDefines.h"
#include "util/StringUtil.h"
#include "glog/logging.h"
#ifdef __USE_L5
#include "util/l5_endpoint_provider.h"
#endif

using std::string;
using std::size_t;

static pthread_mutex_t *lockarray;

static void lock_callback(int mode, int type, const char *file, int line)
{
    (void)file;
    (void)line;
    if(mode & CRYPTO_LOCK) {
        pthread_mutex_lock(&(lockarray[type]));
    }
    else {
        pthread_mutex_unlock(&(lockarray[type]));
    }
}

static unsigned long thread_id(void)
{
    unsigned long ret;

    ret=(unsigned long)pthread_self();
    return ret;
}

static void init_locks(void)
{
    int i;

    lockarray=(pthread_mutex_t *)OPENSSL_malloc(CRYPTO_num_locks() *
            sizeof(pthread_mutex_t));
    for(i=0; i<CRYPTO_num_locks(); i++) {
        pthread_mutex_init(&(lockarray[i]), NULL);
    }

    CRYPTO_set_id_callback((unsigned long (*)())thread_id);
    CRYPTO_set_locking_callback(lock_callback);
}

static void kill_locks(void)
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    for(i=0; i<CRYPTO_num_locks(); i++)
        pthread_mutex_destroy(&(lockarray[i]));

    OPENSSL_free(lockarray);
}

static void __attribute__((constructor)) _curl_init() {
    curl_global_init(CURL_GLOBAL_ALL);                                              
    init_locks();
}                                                                                   

static void __attribute__((destructor)) _curl_fini() {                              
    curl_global_cleanup();                                                          
    kill_locks();
}

namespace qcloud_cos {

    size_t HttpSender::CurlWriter(void *buffer, size_t size, size_t count, void *stream) {
        string *pstream = static_cast<string *>(stream);
        (*pstream).append((char *)buffer, size * count);
        return size * count;
    }

    /*
     * 生成一个easy curl对象，并设置一些公共值
     */
    CURL *HttpSender::CurlEasyHandler(const string &url, string *rsp,
            bool is_post) {
        CURL *easy_curl = curl_easy_init();

        uint64_t conn_timeout = CosSysConfig::getTimeoutInms();
        uint64_t global_timeout = CosSysConfig::getGlobalTimeoutInms();
        curl_easy_setopt(easy_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(easy_curl, CURLOPT_NOSIGNAL, 1);
        // TODO(rabbitliu) 是否需要保护，如最少30s
        curl_easy_setopt(easy_curl, CURLOPT_TIMEOUT_MS, global_timeout);
        curl_easy_setopt(easy_curl, CURLOPT_CONNECTTIMEOUT_MS, conn_timeout);
        curl_easy_setopt(easy_curl, CURLOPT_SSL_VERIFYHOST, 0);
        curl_easy_setopt(easy_curl, CURLOPT_SSL_VERIFYPEER, 1);
        //curl_easy_setopt(easy_curl, CURLOPT_VERBOSE, 1);
        //curl_easy_setopt(easy_curl, CURLOPT_HEADER, 1);

        if (is_post) {
            curl_easy_setopt(easy_curl, CURLOPT_POST, 1);
        }

        curl_easy_setopt(easy_curl, CURLOPT_WRITEFUNCTION, CurlWriter);
        curl_easy_setopt(easy_curl, CURLOPT_WRITEDATA, rsp);

        return easy_curl;
    }

    struct curl_slist* HttpSender::SetCurlHeaders(CURL *curl_handler, const std::map<string, string> &user_headers) {
        struct curl_slist *header_lists = NULL;
        header_lists = curl_slist_append(header_lists, "Accept: */*");
        //header_lists = curl_slist_append(header_lists, "Host: web.file.myqcloud.com");
        header_lists = curl_slist_append(header_lists, "Connection: Keep-Alive");
        header_lists = curl_slist_append(header_lists, "User-Agent: cos-cpp-sdk-v4.2");

        std::map<string, string>::const_iterator it = user_headers.begin();
        string header_key, header_value, full_str;
        for (; it != user_headers.end(); ++it) {
            header_key = it->first;
            header_value = it->second;
            full_str = header_key + ": " + header_value;
            header_lists = curl_slist_append(header_lists, full_str.c_str());
        }
        curl_easy_setopt(curl_handler, CURLOPT_HTTPHEADER, header_lists);
        return header_lists;
    }

    string HttpSender::SendGetRequest(const string url,
            const std::map<string, string> &user_headers,
            const std::map<string, string> &user_params) {
        string user_params_str = "";
        string param_key = "";
        string param_value = "";
        std::map<string, string>::const_iterator it = user_params.begin();
        for (; it != user_params.end(); ++it) {
            if (!user_params_str.empty()) {
                user_params_str += '&';
            }
            param_key = it->first;
            param_value = it->second;
            user_params_str += param_key + "=" + param_value;
        }

        string full_url(url);
        if (full_url.find('?') ==string::npos) {
            full_url += "?" + user_params_str;
        } else {
            full_url += "&" + user_params_str;
        }

        VLOG(5) << "sendGetRequest full url : " << full_url;

        string response = "";
        CURL* get_curl = CurlEasyHandler(full_url, &response, false);
        curl_slist* header_lists = SetCurlHeaders(get_curl, user_headers);

        //int64_t start = GetTimeStampInUs();
        CURLcode ret_code = curl_easy_perform(get_curl);
        //int64_t time_cost_in_us = GetTimeStampInUs() - start;

        curl_slist_free_all(header_lists);
        curl_easy_cleanup(get_curl);


        if (ret_code != CURLE_OK) {
            VLOG(2) << "SendGetRequest error! full_url: " << full_url << ", http_ret:" << ret_code;
#ifdef __USE_L5
            int64_t l5_modid = CosSysConfig::getL5Modid();
            int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
            L5EndpointProvider::UpdateRouterResult(full_url, l5_modid,
                    l5_cmdid,
                    time_cost_in_us, -1);
#endif
            return CosResult(NETWORK_ERROR_CODE,NETWORK_ERROR_DESC).toJsonString();
        }
#ifdef __USE_L5
        int64_t l5_modid = CosSysConfig::getL5Modid();
        int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
        L5EndpointProvider::UpdateRouterResult(full_url, l5_modid, l5_cmdid,
                time_cost_in_us, 0);
#endif
        return response;
    }

    int HttpSender::SendGetRequest(string* pRsp, const string& url,
            const std::map<string, string> &user_headers,
            const std::map<string, string> &user_params) {
        string user_params_str = "";
        string param_key = "";
        string param_value = "";
        std::map<string, string>::const_iterator it = user_params.begin();
        for (; it != user_params.end(); ++it) {
            if (!user_params_str.empty()) {
                user_params_str += '&';
            }
            param_key = it->first;
            param_value = it->second;
            user_params_str += param_key + "=" + param_value;
        }
        string full_url(url);
        if (!user_params_str.empty())
        {
            if (full_url.find('?') ==string::npos) {
                full_url += "?" + user_params_str;
            } else {
                full_url += "&" + user_params_str;
            }
        }

        VLOG(5) << "sendGetRequest full url : " << full_url;

        CURL* get_curl = CurlEasyHandler(full_url, pRsp, false);
        curl_slist* header_lists = SetCurlHeaders(get_curl, user_headers);
        CURLcode ret_code = curl_easy_perform(get_curl);

        long http_code = -1;
        if (ret_code == CURLE_OK) {
            ret_code = curl_easy_getinfo(get_curl, CURLINFO_RESPONSE_CODE , &http_code);
        } else {
            VLOG(2) << "SendGetRequest error! full_url: " << full_url << ", http_ret:" << ret_code;
        }

        curl_slist_free_all(header_lists);
        curl_easy_cleanup(get_curl);

        return http_code;
    }

    string HttpSender::SendJsonPostRequest(const string url,
            const std::map<string, string> &user_headers,
            const std::map<string, string> &user_params) {
        string response = "";
        CURL* json_post_curl = CurlEasyHandler(url, &response, true);

        std::map<string, string> user_headers_cp = user_headers;
        user_headers_cp["Content-Type"] = "application/json";
        curl_slist* header_lists = SetCurlHeaders(json_post_curl, user_headers_cp);

        Json::Value param_json;
        std::map<string, string>::const_iterator it = user_params.begin();
        for (; it != user_params.end(); ++it) {
            param_json[it->first] = it->second;
        }
        Json::FastWriter json_writer;
        string param_str = json_writer.write(param_json);
        curl_easy_setopt(json_post_curl, CURLOPT_POSTFIELDS, param_str.c_str());

        //int64_t start = GetTimeStampInUs();
        CURLcode ret_code = curl_easy_perform(json_post_curl);
        //int64_t time_cost_in_us = GetTimeStampInUs() - start;

        curl_slist_free_all(header_lists);
        curl_easy_cleanup(json_post_curl);


        if (ret_code != CURLE_OK) {
            VLOG(2) << "SendJsonPostRequest error! url: " << url << ", http_ret:" << ret_code;
#ifdef __USE_L5
            int64_t l5_modid = CosSysConfig::getL5Modid();
            int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
            L5EndpointProvider::UpdateRouterResult(url, l5_modid,l5_cmdid,
                    time_cost_in_us, -1);
#endif
            return CosResult(NETWORK_ERROR_CODE,NETWORK_ERROR_DESC).toJsonString();
        }

#ifdef __USE_L5
        int64_t l5_modid = CosSysConfig::getL5Modid();
        int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
        L5EndpointProvider::UpdateRouterResult(url, l5_modid, l5_cmdid,
                time_cost_in_us, 0);
#endif
        return response;
    }

    string HttpSender::SendJsonBodyPostRequest(const string url,const std::string& jsonBody,
            const std::map<string, string> &user_headers) {
        string response = "";
        CURL* json_post_curl = CurlEasyHandler(url, &response, true);

        std::map<string, string> user_headers_cp = user_headers;
        user_headers_cp["Content-Type"] = "application/json";
        curl_slist* header_lists = SetCurlHeaders(json_post_curl, user_headers_cp);

        curl_easy_setopt(json_post_curl, CURLOPT_POSTFIELDS, jsonBody.c_str());

        //int64_t start = GetTimeStampInUs();
        CURLcode ret_code = curl_easy_perform(json_post_curl);
        //int64_t time_cost_in_us = GetTimeStampInUs() - start;

        curl_slist_free_all(header_lists);
        curl_easy_cleanup(json_post_curl);


        if (ret_code != CURLE_OK) {
            VLOG(2) << "SendJsonBodyPostRequest error! url: " << url << ", http_ret:" << ret_code;
#ifdef __USE_L5
            int64_t l5_modid = CosSysConfig::getL5Modid();
            int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
            L5EndpointProvider::UpdateRouterResult(url, l5_modid,l5_cmdid,
                    time_cost_in_us, -1);
#endif
            return CosResult(NETWORK_ERROR_CODE,NETWORK_ERROR_DESC).toJsonString();
        }

#ifdef __USE_L5
        int64_t l5_modid = CosSysConfig::getL5Modid();
        int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
        L5EndpointProvider::UpdateRouterResult(url, l5_modid, l5_cmdid,
                time_cost_in_us, 0);
#endif
        return response;
    }

    string HttpSender::SendSingleFilePostRequest(const string &url,
            const std::map<string, string> &user_headers,
            const std::map<string, string> &user_params,
            const unsigned char* fileContent,
            const unsigned int fileContent_len) {

        struct curl_httppost *firstitem = NULL;
        struct curl_slist *header_lists = NULL;
        string response = "";
        CURL *file_curl = PrepareMultiFormDataCurl(url,
                user_headers,
                user_params,
                fileContent,
                fileContent_len,
                firstitem,
                header_lists,
                &response);
        //int64_t start = GetTimeStampInUs();
        CURLcode ret_code = curl_easy_perform(file_curl);
        //int64_t time_cost_in_us = GetTimeStampInUs() - start;

        curl_formfree(firstitem);
        curl_slist_free_all(header_lists);
        curl_easy_cleanup(file_curl);


        if (ret_code != CURLE_OK) {
            VLOG(2) << "SendSingleFilePostRequest error! url: " << url << ", http_ret:" << ret_code;
#ifdef __USE_L5
            int64_t l5_modid = CosSysConfig::getL5Modid();
            int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
            L5EndpointProvider::UpdateRouterResult(url, l5_modid,l5_cmdid,
                    time_cost_in_us, -1);
#endif
            return CosResult(NETWORK_ERROR_CODE,NETWORK_ERROR_DESC).toJsonString();
        }

#ifdef __USE_L5 
        int64_t l5_modid = CosSysConfig::getL5Modid();
        int64_t l5_cmdid = CosSysConfig::getL5Cmdid();
        L5EndpointProvider::UpdateRouterResult(url, l5_modid, l5_cmdid,
                time_cost_in_us, 0);
#endif
        return response;
    }

    CURL *HttpSender::PrepareMultiFormDataCurl(const string &url,
            const std::map<string, string> &user_headers,
            const std::map<string, string> &user_params,
            const unsigned char* fileContent,
            const unsigned int fileContent_len,
            struct curl_httppost* &firstitem,
            struct curl_slist* &header_lists,
            string *response) {

        struct curl_httppost *lastitem = NULL;
        string param_key = "";
        string param_value = "";
        std::map<string, string>::const_iterator it = user_params.begin();
        for (; it != user_params.end(); ++it) {
            param_key = it->first;
            param_value = it->second;
            curl_formadd(&firstitem, &lastitem,
                    CURLFORM_COPYNAME, param_key.c_str(),
                    CURLFORM_COPYCONTENTS, param_value.c_str(),
                    CURLFORM_END);
        }

        if (fileContent != NULL && fileContent_len != 0) {
            curl_formadd(&firstitem, &lastitem,
                    CURLFORM_COPYNAME, "filecontent",
                    CURLFORM_BUFFER, "data",
                    CURLFORM_BUFFERPTR, fileContent,
                    CURLFORM_BUFFERLENGTH, (long) fileContent_len,
                    CURLFORM_END);
        }

        CURL* file_curl = CurlEasyHandler(url, response, true);

        header_lists = SetCurlHeaders(file_curl, user_headers);

        curl_easy_setopt(file_curl, CURLOPT_HTTPPOST, firstitem);
        return file_curl;
    }

    string HttpSender::SendFileParall(const string url,
            const std::map<string, string> user_headers,
            const std::map<string, string> user_params,
            const string localFileName,
            unsigned long offset,
            const unsigned long sliceSize) {

        string final_response = "";
        bool error_occur = false;

        CURLM *multi_curl = curl_multi_init();

        std::ifstream fileInput(localFileName.c_str(),
                std::ios::in | std::ios::binary);
        fileInput.seekg(0, std::ios::end);
        unsigned long file_len = fileInput.tellg();
        fileInput.seekg(offset, std::ios::beg);

        const unsigned int max_parall_num = 20;
        unsigned char *sliceContentArr[max_parall_num];
        for (unsigned int i = 0; i < max_parall_num; ++i) {
            sliceContentArr[i] = new unsigned char[sliceSize];
        }

        // 用于保存easy_handler
        CURL *easyHandlerArr[max_parall_num];
        memset(easyHandlerArr, 0, sizeof(easyHandlerArr));
        // 用于保存first_item
        struct curl_httppost *firstitemArr[max_parall_num];
        memset(firstitemArr, 0, sizeof(firstitemArr));
        // 用于保存curl_slist
        struct curl_slist* header_listsArr[max_parall_num];
        memset(header_listsArr, 0, sizeof(header_listsArr));
        // 用来保存每一个easy_handler的返回值
        string easyResponseArr[max_parall_num];

        // 用来记录每一个分片的头部信息
        std::map<string, string> slice_params = user_params;

        while(offset < file_len) {
            unsigned int easy_handler_count = 0;
            while(easy_handler_count < max_parall_num) {
                string offset_string = StringUtil::Uint64ToString(offset);
                slice_params["offset"] = offset_string;
                fileInput.read((char *)sliceContentArr[easy_handler_count], sliceSize);
                // byte read len
                unsigned long byte_read_len = 0;
                byte_read_len = fileInput.gcount();

                easyHandlerArr[easy_handler_count] = PrepareMultiFormDataCurl(url,
                        user_headers,
                        slice_params,
                        sliceContentArr[easy_handler_count],
                        byte_read_len,
                        firstitemArr[easy_handler_count],
                        header_listsArr[easy_handler_count],
                        &(easyResponseArr[easy_handler_count]));
                curl_multi_add_handle(multi_curl, easyHandlerArr[easy_handler_count]);

                ++easy_handler_count;
                offset += byte_read_len;
                if (offset >= file_len) {
                    break;
                }
            }

            int easy_handler_running = 0;
            curl_multi_perform(multi_curl, &easy_handler_running);

            do {
                int numfds = 0;
                CURLMcode res = curl_multi_wait(multi_curl, NULL, 0, MAX_WAIT_MSECS, &numfds);
                if (res != CURLM_OK) {
                    std::cerr << "error: curl_multi_wait() returned " << res << std::endl;
                }
                curl_multi_perform(multi_curl, &easy_handler_running);
            } while(easy_handler_running);

            CURLMsg *msg = NULL;
            int msgs_left = 0;
            CURLcode return_code = CURLE_OK;
            int http_status_code;
            while((msg = curl_multi_info_read(multi_curl, &msgs_left))) {
                if (msg->msg == CURLMSG_DONE) {
                    CURL *easy_handler_over = msg->easy_handle;
                    unsigned int j = 0;
                    for(; j < max_parall_num; ++j) {
                        if (easyHandlerArr[j] == easy_handler_over) {
                            break;
                        }
                    }
                    string slice_response = easyResponseArr[j];
                    struct curl_slist *slice_header_slist = header_listsArr[j];
                    header_listsArr[j] = NULL;
                    struct curl_httppost *slice_firstitem = firstitemArr[j];
                    firstitemArr[j] = NULL;

                    return_code = msg->data.result;
                    if (return_code != CURLE_OK) {
                        std::cerr << "slice CURL error code:" << msg->data.result << std::endl;
                        error_occur = true;
                        break;
                    }

                    http_status_code = 0;
                    curl_easy_getinfo(easy_handler_over, CURLINFO_RESPONSE_CODE, &http_status_code);
                    if (http_status_code != 200) {
                        std::cerr << "slice CURL get failed http status code " << http_status_code << std::endl;
                        error_occur = true;
                        break;
                    }

                    Json::Reader reader;
                    Json::Value json_object;
                    if (!reader.parse(slice_response.c_str(), json_object)) {
                        std::cerr << "slice CURL return string not json format!" << slice_response << std::endl;
                        error_occur = true;
                        break;
                    }

                    if (final_response.empty()) {
                        final_response = slice_response;
                    }

                    int code_member = json_object["code"].asInt();
                    if (code_member != 0) {
                        final_response = slice_response;
                    }

                    Json::Value data_member = json_object["data"];
                    if (!data_member["access_url"].isNull()) {
                        final_response = slice_response;
                    }

                    curl_multi_remove_handle(multi_curl, easy_handler_over);
                    curl_formfree(slice_firstitem);
                    curl_slist_free_all(slice_header_slist);
                    curl_easy_cleanup(easy_handler_over);
                }
            }

        }

        fileInput.close();
        for (unsigned int i = 0; i < max_parall_num; ++i) {
            delete[] sliceContentArr[i];
        }
        curl_multi_cleanup(multi_curl);
        if (error_occur) {
            return CosResult(NETWORK_ERROR_CODE,NETWORK_ERROR_DESC).toJsonString();
        } else {
            return final_response;
        }
    }


    int64_t HttpSender::GetTimeStampInUs() {
        // 构造时间
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
    }
}
