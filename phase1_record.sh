#!/usr/bin/env bash
# phase1_record.sh
# 同时采集 Insta360 X4 和 Livox Mid360 数据，写入同一个 .hera 文件
#
# 用法：
#   ./phase1_record.sh [输出目录]
#
# 若未指定输出目录，默认在脚本所在目录下按时间命名的子目录内保存数据。
# 按 Ctrl+C 停止采集。
#
# 环境变量：
#   LIVOX_CONFIG_PATH         Livox SDK2 的 JSON 配置文件绝对路径（必须设置，或见下方自动查找）
#   HERA_DEVICE_PLUGIN_PATH   设备插件目录（默认：build_livox_phase1/device）
#   HERA_DEVICE_LOAD_DRIVER   是否加载驱动插件（1=是，留空/0=否，默认 1）

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_livox_phase1"
RECORD_BIN="${BUILD_DIR}/device/hera-device-record"
PLUGIN_DIR="${BUILD_DIR}/device"
CONFIG_TEMPLATE="${SCRIPT_DIR}/device/src/config_phase1_combined.json"

# ── Livox SDK2 配置文件路径 ───────────────────────────
# 优先从环境变量读取，否则在常见位置自动查找
if [[ -z "${LIVOX_CONFIG_PATH:-}" ]]; then
    for _candidate in \
        "${SCRIPT_DIR}/device/src/mid360_config.json" \
        "${SCRIPT_DIR}/../../../docs/Livox-SDK2/samples/logger/mid360_config.json" \
        "/usr/local/share/livox/mid360_config.json" \
        "/opt/livox-sdk2/samples/logger/mid360_config.json"; do
        if [[ -f "${_candidate}" ]]; then
            LIVOX_CONFIG_PATH="$(realpath "${_candidate}")"
            break
        fi
    done
fi
if [[ -z "${LIVOX_CONFIG_PATH:-}" ]]; then
    echo "[错误] 未找到 Livox SDK2 配置文件。请设置环境变量："
    echo "       export LIVOX_CONFIG_PATH=/path/to/mid360_config.json"
    exit 1
fi
echo "Livox 配置  : ${LIVOX_CONFIG_PATH}"

# ── 输出目录 ─────────────────────────────────────────
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
OUTPUT_DIR="$(realpath -m "${1:-${SCRIPT_DIR}/data/${TIMESTAMP}_phase1}")"
mkdir -p "${OUTPUT_DIR}"
echo "输出目录    : ${OUTPUT_DIR}"

# ── 生成运行时配置（替换 __LIVOX_CONFIG_PATH__ 占位符）──
CONFIG_RUNTIME="${OUTPUT_DIR}/.config_phase1_runtime.json"
sed "s|__LIVOX_CONFIG_PATH__|${LIVOX_CONFIG_PATH}|g" \
    "${CONFIG_TEMPLATE}" > "${CONFIG_RUNTIME}"

# ── 环境变量 ─────────────────────────────────────────
export HERA_DEVICE_PLUGIN_PATH="${HERA_DEVICE_PLUGIN_PATH:-${PLUGIN_DIR}}"
export HERA_DEVICE_LOAD_DRIVER="${HERA_DEVICE_LOAD_DRIVER:-1}"

# ── 检查前置条件 ─────────────────────────────────────
if [[ ! -x "${RECORD_BIN}" ]]; then
    echo "[错误] 采集程序未找到: ${RECORD_BIN}"
    echo "       请先编译: cmake --build build_livox_phase1 -j4 --target hera-device-record"
    exit 1
fi

# ── 检查 sudo 权限 ─────────────────────────────────────
# Insta360 必须以 root 运行（访问 USB 设备需要权限 + 禁用 autosuspend）
if ! sudo -n true 2>/dev/null; then
    echo "[sudo] 需要 sudo 权限启动 Insta360，请输入密码..."
    sudo true || { echo "[错误] sudo 认证失败，无法启动 Insta360"; exit 1; }
fi

# ── 禁用 USB autosuspend：模块级 + 已连接设备级 ──────────
# 仅设置模块级参数只对新枚举的设备生效；已连接设备需要额外设置 sysfs power/control
# 参考 SDK 官方示例中的 USB 要求（camera open 前 USB 必须处于 active 状态）
USB_AUTOSUSPEND_MODULE="/sys/module/usbcore/parameters/autosuspend"
INSTA_VID="2e1a"

sudo sh -c "echo -1 > '${USB_AUTOSUSPEND_MODULE}'" 2>/dev/null \
    && echo "[USB] 模块级 autosuspend 已设为 -1" \
    || echo "[警告] 无法写入 autosuspend 模块参数"

INSTA_SYSFS_PATH=""
for dev in /sys/bus/usb/devices/*/; do
    vid_file="${dev}idVendor"
    if [[ -f "${vid_file}" ]] && [[ "$(cat "${vid_file}" 2>/dev/null)" == "${INSTA_VID}" ]]; then
        INSTA_SYSFS_PATH="${dev}"
        sudo sh -c "echo on  > '${dev}power/control' && echo 0 > '${dev}power/autosuspend_delay_ms'" 2>/dev/null \
            && echo "[USB] Insta360 设备级 power/control 已设为 on (${dev})" \
            || echo "[警告] 无法设置设备级 power/control: ${dev}"
    fi
done

if [[ -z "${INSTA_SYSFS_PATH}" ]]; then
    echo "[警告] 未找到 VID=${INSTA_VID} 的 USB 设备，Insta360 可能未连接"
fi

# ── USB 设备重置（关键）──────────────────────────────────────────────────────
# 上一次因 SIGKILL 或 camera->Close() 超时退出后，相机固件的 USB 会话状态机
# 可能停留在 "session open" 状态，导致下次 Open() 的 Synchronize() 永远超时。
# 在每次启动前做软重置，强制相机 USB 控制器重新初始化，恢复干净状态。
for dev in /sys/bus/usb/devices/*/; do
    vid_file="${dev}idVendor"
    if [[ -f "${vid_file}" ]] && [[ "$(cat "${vid_file}" 2>/dev/null)" == "${INSTA_VID}" ]]; then
        _bus=$(cat "${dev}busnum" 2>/dev/null)
        _devnum=$(cat "${dev}devnum" 2>/dev/null)
        _devpath=$(printf '/dev/bus/usb/%03d/%03d' "$_bus" "$_devnum")
        if [[ -c "${_devpath}" ]]; then
            sudo usbreset "${_devpath}" 2>/dev/null \
                    && echo "[USB] Insta360 USB 重置完成 (${_devpath})" \
                    || echo "[警告] USB 重置失败（需要 sudo）: ${_devpath}"
        fi
    fi
done

# 重置后等待设备重新枚举并稳定（典型 re-enum 需要 2-3 秒）
echo "[USB] 等待相机重新枚举..."
sleep 4

# ── 信号处理 ─────────────────────────────────────────
RECORD_PID=0

cleanup() {
    echo ""
    echo "收到停止信号，正在停止采集..."
    # 进程以 root 运行（需要 USB 访问），须用 sudo kill
    [[ $RECORD_PID -ne 0 ]] && sudo kill -INT "${RECORD_PID}" 2>/dev/null || true

    # camera_->Close() 可能阻塞（SDK 内部等待 USB 响应），最多等待 15 秒后强制终止
    local waited=0
    while [[ $waited -lt 15 ]] && sudo kill -0 "${RECORD_PID}" 2>/dev/null; do
        sleep 1
        waited=$((waited + 1))
    done
    if sudo kill -0 "${RECORD_PID}" 2>/dev/null; then
        echo "[警告] 进程在 15 秒内未退出（camera->Close() 阻塞），强制终止"
        sudo kill -9 "${RECORD_PID}" 2>/dev/null || true
        sleep 1
        # 强制终止后重置 USB，恢复相机会话状态，避免下次启动 Synchronize 超时
        for dev in /sys/bus/usb/devices/*/; do
            vid_file="${dev}idVendor"
            if [[ -f "${vid_file}" ]] && [[ "$(cat "${vid_file}" 2>/dev/null)" == "${INSTA_VID}" ]]; then
                _bus=$(cat "${dev}busnum" 2>/dev/null)
                _devnum=$(cat "${dev}devnum" 2>/dev/null)
                _devpath=$(printf '/dev/bus/usb/%03d/%03d' "$_bus" "$_devnum")
                [[ -c "${_devpath}" ]] && sudo usbreset "${_devpath}" 2>/dev/null \
                    && echo "[USB] 清理时重置 Insta360 USB (${_devpath})"
            fi
        done
    fi

    wait "${RECORD_PID}" 2>/dev/null || true
    echo "✓ 采集已停止，数据保存在: ${OUTPUT_DIR}"
}
trap cleanup INT TERM

# ── 启动采集 ─────────────────────────────────────────────────────────────────
echo "[$(date '+%H:%M:%S')] 启动采集（Insta360 X4 + Livox Mid360，以 root 运行）..."
# 单进程同时管理两个设备，写入同一个 .hera 文件
# -E 保留 HERA_DEVICE_PLUGIN_PATH / HERA_DEVICE_LOAD_DRIVER 等环境变量
sudo -E sh -c "echo -1 > /sys/module/usbcore/parameters/autosuspend && \
    cd '${OUTPUT_DIR}' && '${RECORD_BIN}' '${CONFIG_RUNTIME}'" \
    > "${OUTPUT_DIR}/record.log" 2>&1 &
RECORD_PID=$!

echo ""
echo "================================================================"
echo " 采集进行中（按 Ctrl+C 停止）"
echo "   PID    : ${RECORD_PID}"
echo "   日志   : record.log"
echo "   配置   : config_phase1_combined.json"
echo "================================================================"

# 等待采集进程结束
wait "${RECORD_PID}" || true

echo ""
echo "================================================================"
echo " 采集完成，生成的文件："
ls -lh "${OUTPUT_DIR}"/*.hera 2>/dev/null || echo "  （未检测到 .hera 文件）"
echo "================================================================"
