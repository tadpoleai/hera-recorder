# Nginx 配置指南

## 安装Nginx

运行 `deploy/install-nginx.sh`

## 配置Nginx

1. 修改`/etc/nginx/nginx.conf`，确认`http`包含以下语句

    ```plain-text
    include /etc/nginx/sites-enabled/*;
    ```

1. 删除`/etc/nginx/sites-enabled/*`

1. 将本仓库内的[`deploy/nginx/hera-client`](nginx/hera-client)复制到`/etc/nginx/sites-enabled/`

1. 新建文件夹`/var/www/hera-client/`
