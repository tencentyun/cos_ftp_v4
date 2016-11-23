#!/bin/bash

cur_dir=$(cd `dirname $0`;pwd)
cd ${cur_dir}

op_log=${cur_dir}/../log/op/ftp_op.log

while [ "1" = "1" ]
do
    cur_clear_crontab=`crontab -l | grep vsftpd_log_clear.sh`
    if [[ ${cur_pid} == "" ]]; then
       echo "0 0 * * * sh ${cur_dir}/cos_ftp_log_clear.sh &> /dev/null" > ./clear_crontab.cron 
       crontab ./clear_crontab.cron
       rm -f ./clear_crontab.cron
    fi
   
    cur_pid=`ps ajx | grep vsftpd | awk '{if($1 == 1) {print $2}}'`
    if [[ ${cur_pid} == "" ]]; then
       cur_date=`date "+%Y-%m-%d %H:%M:%S"`
       echo "${cur_date}:try to start vsftpd" >> ${op_log}
       sh ./start_cos_ftp.sh
    fi
    sleep 1
done
