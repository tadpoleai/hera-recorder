# Hera Docker 镜像使用指南

## 准备步骤

### 登录公司 Docker 服务器

`sudo docker login 172.2.2.11:5000/hdmap_2021/hera-recorder:latest`

Username: `admin`

Passwaord: `wayz@1234`

### 拉取最新镜像

`sudo docker pull 172.2.2.11:5000/hdmap_2021/hera-recorder:latest`

## 使用 Docker 镜像构建 Hera

确保当前目录为工程根目录

`sudo docker run -it --rm -v $(pwd):/workspace/hera-recorder 172.2.2.11:5000/hdmap_2021/hera-recorder:latest`

然后执行 `build_all.sh` 或者 `scripts/build_amd64.sh` 等命令

## 编译 Docker 镜像

`sudo docker build -t 172.2.2.11:5000/hdmap_2021/hera-recorder:<tag> .`

若缺少文件, 请自行从 NFS 下载

## 上传到公司测试环境 Docker 服务器

`sudo docker tag 172.2.2.11:5000/hdmap_2021/hera-recorder:<tag> 172.2.2.11:5000/hdmap_2021/hera-recorder:latest`

`sudo docker push 172.2.2.11:5000/hdmap_2021/hera-recorder:latest`

## 上传到公司生产环境 Docker 服务器

上传前请保证镜像内运行 `build_all.sh` 能通过并编译出可以运行的程序, 以免自动化测试失败

`sudo docker tag registry.newayz.com/autonomy/hera-recorder:latest registry.newayz.com/autonomy/hera-recorder:latest`

`sudo docker push registry.newayz.com/autonomy/hera-recorder:latest`
