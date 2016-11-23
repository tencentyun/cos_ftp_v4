#!/bin/bash

# kill daemon
ps aux | grep "cos_ftp_daemon.sh" | grep -v grep | awk '{print $2}' | xargs -I{} kill -9 {}

# kill vsftpd
pkill vsftpd
