# 编译 hera-recorder docker 镜像

## 登录公司 docker 服务器

`docker login registry.newayz.com/autonomy/hera-recorder:latest`

`Username: autonomy`

`Passwaord:auto123`

## 拉取最新 hera-recorder docker 镜像

`sudo docker pull registry.newayz.com/autonomy/hera-recorder:latest`

## 编译 docker 镜像

`sudo docker build -t registry.newayz.com/autonomy/hera-recorder:latest .`

## 测试 docker 镜像

`cd .. `(移动至工程根目录)

`sudo docker run -t --rm -v $(pwd):/workspace/hera-recorder registry.newayz.com/autonomy/hera-recorder:latest`

## 上传 docker 镜像

### 上传至公司 docker registry

`sudo docker push registry.newayz.com/autonomy/hera-recorder:latest`

tag可以从Dockerfile第5行得到, 每次修改Dockerfile, 请将tag增加0.1
`sudo docker tag registry.newayz.com/autonomy/hera-recorder:latest registry.newayz.com/autonomy/hera-recorder:<tag>`

`sudo docker push registry.newayz.com/autonomy/hera-recorder:<tag>`

### 上传至自动化测试 docker registry

`sudo docker tag registry.newayz.com/autonomy/hera-recorder:latest registry.newayz.com/autonomy/hera-recorder:latest`
`sudo docker push registry.newayz.com/autonomy/hera-recorder:latest`
