# SFTP

![Platform](https://img.shields.io/badge/paltform-win10_x64-brightgreen)
![Qt Version](https://img.shields.io/badge/_Qt_-6.5.3-yellowgreen)
![Build](https://img.shields.io/badge/build-MSVC_2019_x64-blue)

#### 一、介绍
​		文件传输工具类

#### 二、软件架构
​		`Qt` + `libcurl`

#### 三、新建 SFTP 用户

```shell
sudo adduser Mtr1994 -s /sbin/nologin  （添加一个用户）
```

```shell
sudo groupadd sftp_users  （添加一个组）
```

```shell
sudo usermod -g sftp_users Mtr1994（修改用户的组）
```

```shell
sudo usermod -d /new/directory -m Mtr1994（修改用户主目录）
```

```shell
cat /etc/passwd （查看用户信息）
cat /etc/group（查看组信息）
```

```shell
// SSH 服务的安装、控制
sudo apt-get install openssh-server
sudo service ssh start
sudo service ssh stop
ssh -V
```

```shell
// ssh 登录问题，subsystem 使用默认的 sftp-server，如果使用的 internal-sftp, 即使是nologin也能正常连接
sudo vi /etc/ssh/sshd_config

// 注释这一句
Subsystem sftp /usr/lib/openssh/sftp-server

// 添加这一句
Subsystem sftp internal-sftp

// 添加如下内容
Match Group sftp_users
        ChrootDirectory /home/ftp/%u
        X11Forwarding no
        AllowTcpForwarding no
        PermitTTY no
        ForceCommand internal-sftp
```



> 注意事项，从用户主目录开始，一路往上的文件夹的拥有者都必须是 root 用户，否则无法登录

> 修改用户的组之后，后续上传文件自动创建的文件夹会自动设置用户和用户组
