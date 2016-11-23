// Copyright (c) 2016, Tencent Inc.
// All rights reserved.
//
// Author: rabbitliu <rabbitliu@tencent.com>
// Created: 06/06/16
// Description:

#include "util/l5_endpoint_provider.h"

#include <sys/time.h>
#include <stdlib.h>

#include <iostream>
#include <sstream>

#include "l5/qos_client.h"
#include "CosDefines.h"

namespace qcloud_cos {

bool L5EndpointProvider::GetEndPoint(int64_t modid, int64_t cmdid, std::string* endpoint) {
    if (endpoint == NULL) {
        return false;
    }
    std::string err_msg;
    QOSREQUEST qos_request;
    qos_request._modid = modid;
    qos_request._cmd = cmdid;
    std::string ret_url;
    static const int kMaxRetryTimes = 3;
    int retry_num = 0;
    int ret = -1;
    // 重试三次
    while (retry_num < kMaxRetryTimes) {
        ++retry_num;
        ret = ApiGetRoute(qos_request, 1, err_msg);
        if (ret >= 0) {
            // 获取成功
            break;
        }
    }
    // 重试三次后依然失败
    if (ret < 0) {
        return false;
    }
    std::stringstream s;
    s << "http://" << qos_request._host_ip << ":"
        << qos_request._host_port << "/files/v2/";
    *endpoint = s.str();
    return true;
}

bool L5EndpointProvider::UpdateRouterResult(const std::string& full_path,
                                            int64_t modid, int64_t cmdid,
                                            int64_t use_time, int ret) {
    // 没走l5，不用上报
    if (modid == -1 || cmdid == -1) {
        return false;
    }

    std::string host;
    int port = 0;
    if (!ParseEndpoint(full_path, &host, &port)) {
        return false;
    }

    QOSREQUEST qos_request;
    qos_request._host_ip = host;
    qos_request._host_port = port;
    qos_request._modid = modid;
    qos_request._cmd = cmdid;
    std::string err_msg;
    // 上报数据
    int update_ret = ApiRouteResultUpdate(qos_request, ret, static_cast<int>(use_time), err_msg);
    return update_ret == 0;
}

// TODO(rabbitliu) 重构这里，将url的计算延后至发送请求时，可以避免从full_url中计算endpoint
bool L5EndpointProvider::ParseEndpoint(const std::string& full_path,
                                       std::string* host, int* port) {
    // full_path: "http://1.2.3.4:8080/files/v1/
    size_t pos_start = full_path.find("//");
    size_t pos_end = full_path.find("/", pos_start + 2);
    if (pos_start == std::string::npos || pos_end == std::string::npos) {
        return false;
    }
    std::string endpoint = full_path.substr(pos_start + 2,
                                            pos_end - pos_start - 2);
    size_t pos = endpoint.find(":");
    if (pos == std::string::npos) {
        return false;
    }
    *host = endpoint.substr(0, pos);
    *port = atoi(endpoint.substr(pos + 1).c_str());
    return true;
}

} // namespace qcloud_cos
