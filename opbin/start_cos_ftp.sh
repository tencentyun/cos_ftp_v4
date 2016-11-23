#!/bin/bash
cur_dir=$(cd `dirname $0`;pwd)
cd ${cur_dir}

export GLOG_log_dir=${cur_dir}/../log/sdk
# DEBUG SET 5, INFO SET 4
export GLOG_v=4
export GLOG_logbufsecs=0 

${cur_dir}/../bin/vsftpd ${cur_dir}/../conf/vsftpd.conf
