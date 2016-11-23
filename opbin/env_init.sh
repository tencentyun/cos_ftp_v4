#!/bin/bash
yum -y groupinstall "Development Tools"
yum -y install cmake
yum -y install boost-devel.x86_64
yum -y install openssl-devel
yum -y install asio-devel
yum -y install libidn-devel

if [ ! -f /usr/lib64/libboost_thread.so ]; then
    ln -s /usr/lib64/libboost_thread-mt.so /usr/lib64/libboost_thread.so
fi
