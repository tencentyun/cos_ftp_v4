#!/bin/bash
# clear ftp log
cur_dir=$(cd `dirname $0`;pwd)
cd ${cur_dir}


# set log saved days, default 7 days
DAYS=7

ftp_log=${cur_dir}/../log/ftp
sdk_log=${cur_dir}/../log/sdk
op_log=${cur_dir}/../log/op

yesterday=`date -d "yesterday" +"%Y%m%d"`

mkdir -p "${ftp_log}/${yesterday}"
mkdir -p "${sdk_log}/${yesterday}"
mkdir -p "${op_log}/${yesterday}"

mv $ftp_log/*.log "${ftp_log}/${yesterday}/"
mv $sdk_log/*.log "${sdk_log}/${yesterday}/"
mv $op_log/*.log "${op_log}/${yesterday}/"

find ${ftp_log} -mtime +$DAYS -exec rm -rf {} \;
find ${sdk_log} -mtime +$DAYS -exec rm -rf {} \;
find ${op_log} -mtime +$DAYS -exec rm -rf {} \;

