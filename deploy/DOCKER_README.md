# 编译 hera-recorder docker 镜像

## docker login registry.newayz.com/autonomy/hera-recorder:latest镜像

    Username: autonomy

    Passwaord:auto123

## 获取最新hera-recorder docker镜像

    sudo docker pull 172.2.2.11:5000/hera-recorder:latest

## 编译docker镜像

    sudo docker build -t 172.2.2.11:5000/hera-recorder:latest .

## 测试docker镜像

    cd .. (移动至工程根目录)
    sudo docker run -t --rm -v $(pwd):/workspace/hera-recorder 172.2.2.11:5000/hera-recorder:latest

## 上传docker镜像

### 上传至公司docker registry

    sudo docker push 172.2.2.11:5000/hera-recorder:latest

    // <tag>可以从Dockerfile第5行得到, 每次修改Dockerfile, 请将tag增加0.1
    sudo docker tag 172.2.2.11:5000/hera-recorder:latest 172.2.2.11:5000/hera-recorder:<tag>
    sudo docker push 172.2.2.11:5000/hera-recorder:<tag>

### 上传至自动化测试docker registry

    sudo docker tag 172.2.2.11:5000/hera-recorder:latest registry.newayz.com/autonomy/hera-recorder:latest
    sudo docker push registry.newayz.com/autonomy/hera-recorder:latest
