#!/bin/bash
cur_dir=$(cd `dirname $0`;pwd)
cd ${cur_dir}

# init env
sh ${cur_dir}/opbin/env_init.sh

# compile sdk
cd ${cur_dir}/dep/cos-cpp-sdk
rm -f CMakeCache.txt
cmake .
make clean && make
cd -
cp -a ./dep/cos-cpp-sdk/lib/* ./lib/
rm -rf ./include/*
cp -a ./dep/cos-cpp-sdk/include/* ./include/

# compile ftp server
cd src
chmod a+x vsf_findlibs.sh
make clean && make
cp -a ./vsftpd ../bin/
cd -
echo "build over"

# modify temp data path
sed -i "s#^cos_user_home_dir.*\$#cos_user_home_dir=$cur_dir/data/#g" conf/vsftpd.conf
sed -i "s#^vsftpd_log_file.*\$#vsftpd_log_file=$cur_dir/log/ftp/vsftpd.log#g" conf/vsftpd.conf
sed -i "s#^xferlog_file.*\$#xferlog_file=$cur_dir/log/ftp/xfer.log#g" conf/vsftpd.conf


# add a user not root
useradd cos_ftp &> /dev/null
chown -R cos_ftp:cos_ftp ./*
