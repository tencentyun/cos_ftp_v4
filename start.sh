#!/bin/bash
cur_dir=$(cd `dirname $0`;pwd)
cd ${cur_dir}

#nohup ${cur_dir}/opbin/cos_ftp_daemon.sh &> ${cur_dir}/log/op/daemon_log.sh &
nohup sh ${cur_dir}/opbin/cos_ftp_daemon.sh &> ${cur_dir}/log/op/ftp_daemon.log &
