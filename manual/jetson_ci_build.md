# hera-daemon Jetson arm64 CI 构建与部署

本文记录通过 GitHub Actions 自动构建 `hera-daemon` arm64 `.deb` 包并部署到 Jetson 的完整流程，以及过程中遇到的所有坑。

> **对应旧文档**：`manual/jetson_deploy.md` 是在 Jetson 上手动编译的方案（已过时），本文是取而代之的 CI 自动打包方案。

---

## 架构概览

```
本地 x86_64 开发机
  └── docker run jetson-builder:r36.4 (QEMU aarch64 仿真) → 本地验证

GitHub Actions (ubuntu-24.04-arm 原生 arm64 runner)
  └── container: ghcr.io/tadpoleai/jetson-builder:r36.4
        ├── build_livox_sdk2.sh       ← clone tadpoleai/Livox-SDK2 并编译安装
        ├── INSTA_SDK_ROOT=/opt/insta_sdk build_arm64.sh   ← 编译 hera-daemon
        ├── INSTA_SDK_ROOT=/opt/insta_sdk make_artifacts.sh arm64
        │     包含: libCameraSDK.so, liblivox_lidar_sdk_shared.so
        ├── make_deb.sh → dist/hera-daemon_<version>_arm64.deb
        ├── upload-artifact (每次都上传，7 天保留)
        └── action-gh-release (仅 v* tag 触发)
```

**关键决策**：
- 不用 x86_64 + apt-multiarch 交叉编译（脆、慢、与真机环境不一致）
- `ubuntu-24.04-arm` runner + `jetson-builder:r36.4` container = runner 与镜像同架构，无需仿真
- 旧的交叉编译 job 已完全删除

---

## 关键资源

| 资源 | 地址 / 路径 |
|------|------------|
| GitHub 仓库 | `github.com/tadpoleai/hera-recorder` |
| jetson-builder 镜像 | `ghcr.io/tadpoleai/jetson-builder:r36.4`（private，通过 `GITHUB_TOKEN` 拉取） |
| jetson-builder Dockerfile | `cloud/docker/Dockerfile.jetson-builder` |
| Livox-SDK2 仓库 | `github.com/tadpoleai/Livox-SDK2` |
| Insta360 SDK（本地） | `/home/fred/Code/sdks/insta_sdk/CameraSDK-2.1.1-jetson/`（已打包进镜像） |
| CI workflow | `.github/workflows/build-daemon.yml` |
| 构建脚本 | `scripts/build_arm64.sh`（接受 `INSTA_SDK_ROOT` env） |
| Livox 构建脚本 | `scripts/build_livox_sdk2.sh`（幂等，已安装则跳过） |
| 产物收集脚本 | `scripts/make_artifacts.sh` |
| 打包脚本 | `scripts/make_deb.sh` |
| Livox 配置模板 | `config/mid360_config.json` → 安装到 `/etc/mid360_config.json` |

---

## SDK 管理

### Insta360 CameraSDK

- **当前方案**：打包进 `jetson-builder:r36.4` 镜像，路径 `/opt/insta_sdk/`
- **运行时库**：`libCameraSDK.so` 由 `make_artifacts.sh` 从 `/opt/insta_sdk/lib/` 复制到 `.deb`，安装到 `/usr/local/lib/`
- **更新镜像**：修改 `cloud/docker/Dockerfile.jetson-builder` 后执行：
  ```bash
  docker build \
    -f cloud/docker/Dockerfile.jetson-builder \
    -t ghcr.io/tadpoleai/jetson-builder:r36.4 \
    /home/fred/Code/sdks/insta_sdk/
  docker push ghcr.io/tadpoleai/jetson-builder:r36.4
  docker tag ghcr.io/tadpoleai/jetson-builder:r36.4 jetson-builder:r36.4
  ```
- **长期计划**：上传 SDK 到 OSS，通过 `INSTA_SDK_URL` 下载（对齐 `cloud/docker/Dockerfile.build-arm64` 的方式）

### Livox SDK2

- **当前方案**：CI 每次从 `github.com/tadpoleai/Livox-SDK2` clone + 原生 aarch64 编译
- **运行时库**：`liblivox_lidar_sdk_shared.so` 由 `make_artifacts.sh` 从 `/usr/local/lib/` 复制到 `.deb`
- **更新 SDK**：
  ```bash
  cd /home/fred/Code/sdks/Livox-SDK2
  # ... 修改 ...
  git add . && git commit -m "..." && git push origin master
  ```
  下次 CI 自动 clone 新版本

---

## 触发 CI 构建

### 手动验证（不发布，任意分支/提交）

```
GitHub → tadpoleai/hera-recorder → Actions → Build Jetson Daemon → Run workflow
```

完成后在 Actions run 页面底部 **Artifacts** 下载 `.deb`（zip 格式，保留 7 天）。

### 正式发布（生成 GitHub Release）

```bash
git tag v4.12.0
git push github v4.12.0
```

CI 自动构建并上传 `.deb` 到 **GitHub Releases**（同时也有 Artifacts 副本）。

---

## 本地验证（不等 CI，QEMU 仿真）

前提：本机已安装 QEMU binfmt（`/proc/sys/fs/binfmt_misc/qemu-aarch64` enabled）。

```bash
# 一次性验证（--rm 容器用完即销毁）
docker run --rm \
  -v "$(pwd):/src" -w /src \
  jetson-builder:r36.4 \
  bash -c '
    set -e
    git config --global --add safe.directory /src
    bash scripts/build_livox_sdk2.sh
    INSTA_SDK_ROOT=/opt/insta_sdk bash scripts/build_arm64.sh 0.0.0-local-verify
    INSTA_SDK_ROOT=/opt/insta_sdk bash scripts/make_artifacts.sh arm64
    mkdir -p dist && bash scripts/make_deb.sh 0.0.0-local-verify artifacts dist arm64
    ls -lh dist/*.deb
  '
```

**注意**：`build_arm64/`、`artifacts/`、`dist/` 会以 root 属主写回宿主机，清理：
```bash
sudo chown -R $USER build_arm64 artifacts dist
```

迭代调试时用长期容器（避免 Livox SDK 每次重编）：
```bash
docker run -d --name jetson-verify -v "$(pwd):/src" -w /src jetson-builder:r36.4 sleep infinity
docker exec jetson-verify bash -c 'git config --global --add safe.directory /src && ...'
docker rm -f jetson-verify
```

---

## 安装后配置

### 1. `/etc/mid360_config.json`（Livox Mid360 必需）

`.deb` 会自动安装模板到 `/etc/mid360_config.json`（conffile，升级不覆盖）。

**必须修改 `host_ip` 为 Jetson 上连接激光雷达那个网口的 IP**：

```bash
sudo nano /etc/mid360_config.json
```

```json
{
  "MID360": {
    "lidar_net_info": {
      "cmd_data_port": 56100,
      "push_msg_port": 56200,
      "point_data_port": 56300,
      "imu_data_port": 56400,
      "log_data_port": 56500
    },
    "host_net_info": [
      {
        "host_ip": "192.168.1.5",    ← 改为 Jetson 实际 IP
        "multicast_ip": "224.1.1.5",
        "cmd_data_port": 56101,
        "push_msg_port": 56201,
        "point_data_port": 56301,
        "imu_data_port": 56401,
        "log_data_port": 56501
      }
    ]
  }
}
```

### 2. `/etc/hera.conf`（daemon 配置）

同样由 `.deb` 安装，通常默认值即可（daemon 名称、监听端口 10093 等）。

---

## .deb 安装后的文件布局

```
/usr/local/bin/
  hera-daemon
  hera-daemon-finder
  hera-replay
  hera-storage-tool
  hera-storage-extract-mid360
  ...

/usr/local/lib/
  libhera-common.so
  libhera-device.so
  libhera-storage.so
  libCameraSDK.so            ← Insta360 运行时库
  liblivox_lidar_sdk_shared.so  ← Livox 运行时库

/usr/local/lib/hera/plugin/
  base/   ← 20 个 *-base.so（所有设备类型注册）
  driver/ ← 15 个 *-driver.so（含 camera-insta, lidar-livox 等）
  upload/ ← 2 个上传插件

/etc/hera.conf              ← daemon 主配置（conffile）
/etc/mid360_config.json     ← Livox SDK2 配置（conffile，需改 host_ip）
/lib/systemd/system/hera-daemon.service
/lib/systemd/system/udiskie.service
/var/hera/data/             ← 录制数据目录
/var/hera/logs/             ← 日志目录
```

---

## 常见问题

| 现象 | 根因 | 解决 |
|------|------|------|
| `Error: denied` 拉取 ghcr.io 镜像 | package 是 private，GITHUB_TOKEN 权限不足 | workflow 已配置 `packages: read` + `credentials`；或把 package 改为 Public |
| `version number does not start with digit` | `workflow_dispatch` 时 `GITHUB_REF_NAME` 是分支名 | 已修复：非 tag 触发时 VERSION 固定为 `0.0.0-ci` |
| `GitHub Releases requires a tag` | `workflow_dispatch` 没有 tag | 已修复：Release 上传步骤加了 `if: startsWith(github.ref, 'refs/tags/v')` |
| `Device type given 'camera/insta' is invalid` | `libCameraSDK.so` 未打包进 `.deb`，运行时 dlopen 失败 | 已修复：`make_artifacts.sh` 从 `INSTA_SDK_ROOT/lib/` 复制到 artifacts |
| Livox 连接时 SIGSEGV（`fread` + `FileReadStream`) | `/etc/mid360_config.json` 不存在，`fopen()` 返回 NULL 传给 rapidjson | 已修复：① 代码加文件存在检查返回清晰错误；② `.deb` 打包配置模板 |
| Livox 连接报 `Livox config file not found` | 安装了新 `.deb` 但未修改 `host_ip`，或配置文件路径不对 | 检查 `/etc/mid360_config.json` 存在且 `host_ip` 为 Jetson 正确 IP |
| CMakeCache 路径冲突 | 旧 `build_arm64/` 在不同挂载路径下生成 | 删除 `build_arm64/` 重跑（容器内 root 属主需用 `docker exec` 删） |
| Livox clone 为空 | `tadpoleai/Livox-SDK2` 仓库无内容 | `cd /home/fred/Code/sdks/Livox-SDK2 && git push origin master` |

---

## ghcr.io 可见性说明

当前 `ghcr.io/tadpoleai/jetson-builder:r36.4` 是 **private**，workflow 通过 `GITHUB_TOKEN` 拉取。
如改为 Public 可简化 workflow（删除 `credentials` 块和 `packages: read`）：

> GitHub → tadpoleai (org) → Packages → jetson-builder → Package settings → Change visibility → Public
