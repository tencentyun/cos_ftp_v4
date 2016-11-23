COS FTP V4 用于通过FTP协议往COS上传和下载文件.

## 支持FTP命令

- put

- get

- mput

- mget

- delete

- mkdir

- ls

- cd

- bye

- quit

- size

  ​

## 适用COS版本
​	4.x 

## 系统要求

​	Linux (推荐腾讯云Centos系列CVM)

## 依赖库

​	针对Centos等使用yum安装的系统，build.sh会自动下载以下依赖。其他系统请自行安装。

```xml
cmake
boost
openssl-devel
asio-devel
libidn-devel
```

## 编译

1. 因为FTP需要使用本地磁盘，因此请将FTP源码程序放在一个存储空间较大的盘。(腾讯云初始的机器购买的数据盘需要手动格式化并挂载, 请参考 https://www.qcloud.com/doc/product/213/2974
2. 以root身份运行build.sh(因为build.sh里会调用yum进行安装依赖库，推荐使用腾讯云主流的Centos系列系统，如果是其他系列系统，如ubuntu，请修改opbin/env_init.sh)

## 配置

配置文件conf/vsftd.conf中的是vsftpd的配置,可以参考以下配置说明, 需要改动的主要是以下两模块


    1. COS账户信息配置
        #cos, set your app info in cos                                                   
        cos_appid=1000000                                                   
        cos_secretid=xxxxxxxxxxxxxxxxxxxxxxxxx                              
        cos_secretkey=xxxxxxxxxxxxxxxxxx 
        # bucket信息，包括bucket的名字，以及bucket所在的区域。目前有效值华南广州(gz), 华东上海(sh), 华北天津(tj)
        cos_bucket=test                                                     
        cos_region=gz
        # domain设置为cos表示通过COS源站下载(推荐服务器为腾讯云机器用户设置)
        # domain设置为CDN表示通过CDN下载(推荐服务器为非腾讯云机器用户设置)
        cos_download_domain=cos                                             
        # 此项不用设置, build.sh脚本会自动设置
        cos_user_home_dir=/home/test/cosftp_data/                                        
    
    2. FTP账户配置(格式-用户名:密码:读写权限. 多个账户用分号分割)
        login_users=user1:pass1:RW;user2:pass2:RW

## 运行
    1.切换到非root身份(目前以root身份运行会存在一些问题),编译过程会自动建立一个cos_ftp用户,因此可以直接使用该用户,通过root切到cos_ftp.(su cos_ftp)
    2. sh start.sh (会启动FTP进程和monitor程序,以及安装自动清理日志的脚本)
    3. 可使用FTP客户端连接server，进行文件的上传与下载

## 停止
    运行 sh stop.sh
