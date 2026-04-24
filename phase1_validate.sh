#!/usr/bin/env bash
# phase1_validate.sh
# 解析并验证单个合并 .hera 文件中的 Insta360 X4 + Livox Mid360 数据
#
# 用法：
#   ./phase1_validate.sh <phase1.hera> [输出目录]
#
# 输出目录默认为当前目录下按时间命名的子目录。
# 脚本会：
#   1. 用 hera-storage-tool 统计 .hera 文件的包数与频率
#   2. 用 hera-storage-extract-insta 提取视频帧 + gyro CSV
#   3. 用 hera-storage-extract-mid360 提取点云 CSV + IMU CSV
#   4. 对所有输出文件做基本完整性检查，打印 PASS/FAIL 汇总

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build_livox_phase1"
STORAGE_TOOL="${BUILD_DIR}/storage/hera-storage-tool"
EXTRACT_INSTA="${BUILD_DIR}/storage/hera-storage-extract-insta"
EXTRACT_MID360="${BUILD_DIR}/storage/hera-storage-extract-mid360"

# ── 参数解析 ─────────────────────────────────────────
if [[ $# -lt 1 ]]; then
    echo "用法: $0 <phase1.hera> [输出目录]"
    echo ""
    echo "示例:"
    echo "  $0 data/session_001/20260422_phase1.hera \\"
    echo "       data/session_001/extracted"
    exit 1
fi

# 校验 .hera 文件确实存在且不是目录
if [[ ! -f "$1" ]]; then
    echo "[错误] 文件不存在: $1"
    echo "       请传入合并的 phase1.hera 文件（由 phase1_record.sh 生成）"
    exit 1
fi
if [[ "$1" != *.hera ]]; then
    echo "[警告] 文件后缀不是 .hera: $1（继续执行）"
fi

PHASE1_HERA="$(realpath "$1")"
TIMESTAMP="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="${2:-${SCRIPT_DIR}/validation_${TIMESTAMP}}"
mkdir -p "${OUT_DIR}"

PASS=0
FAIL=0

# ── 工具函数 ─────────────────────────────────────────
check() {
    local label="$1"; local result="$2"
    if [[ "${result}" == "pass" ]]; then
        echo "  [PASS] ${label}"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] ${label}: ${result}"
        FAIL=$((FAIL + 1))
    fi
}

check_bin() {
    local bin="$1"
    if [[ ! -x "${bin}" ]]; then
        echo "[错误] 工具未找到: ${bin}"
        echo "       请先编译: cmake --build build_livox_phase1 -j4"
        exit 1
    fi
}

# ── 检查工具 ─────────────────────────────────────────
check_bin "${STORAGE_TOOL}"
check_bin "${EXTRACT_INSTA}"
check_bin "${EXTRACT_MID360}"

echo "================================================================"
echo " Phase1 数据验证"
echo "   .hera 文件: ${PHASE1_HERA}"
echo "   输出目录  : ${OUT_DIR}"
echo "================================================================"
echo ""

# ── 第 1 步：存储统计 ─────────────────────────────────
echo ">>> [1/4] 存储文件统计"
echo ""
"${STORAGE_TOOL}" -i "${PHASE1_HERA}" -a 2>&1 | tee "${OUT_DIR}/phase1_stats.txt"
echo ""

# ── 第 2 步：提取 Insta360 数据 ───────────────────────
echo ">>> [2/4] 提取 Insta360 数据"
INSTA_VIDEO="${OUT_DIR}/insta_video.h264"
INSTA_GYRO="${OUT_DIR}/insta_gyro.csv"
"${EXTRACT_INSTA}" "${PHASE1_HERA}" --video "${INSTA_VIDEO}" --gyro "${INSTA_GYRO}" 2>&1 \
    | tee "${OUT_DIR}/insta_extract.log"
echo ""

# ── 第 3 步：提取 Mid360 数据 ─────────────────────────
echo ">>> [3/4] 提取 Mid360 数据"
MID360_POINTS="${OUT_DIR}/mid360_points.csv"
MID360_IMU="${OUT_DIR}/mid360_imu.csv"
"${EXTRACT_MID360}" "${PHASE1_HERA}" --points "${MID360_POINTS}" --imu "${MID360_IMU}" 2>&1 \
    | tee "${OUT_DIR}/mid360_extract.log"
echo ""

# ── 第 4 步：数据完整性检查 ───────────────────────────
echo ">>> [4/4] 数据完整性检查"
echo ""

# Insta360 视频
if [[ -f "${INSTA_VIDEO}" ]]; then
    VIDEO_SIZE=$(wc -c < "${INSTA_VIDEO}")
    if [[ $VIDEO_SIZE -gt 100000 ]]; then
        check "Insta360 视频文件大小 (${VIDEO_SIZE} bytes)" "pass"
    else
        check "Insta360 视频文件大小" "文件过小: ${VIDEO_SIZE} bytes"
    fi
else
    check "Insta360 视频文件存在" "文件不存在: ${INSTA_VIDEO}"
fi

# Insta360 Gyro CSV
if [[ -f "${INSTA_GYRO}" ]]; then
    GYRO_ROWS=$(wc -l < "${INSTA_GYRO}")
    GYRO_HEADER=$(head -1 "${INSTA_GYRO}")
    if [[ $GYRO_ROWS -gt 100 ]] && [[ "${GYRO_HEADER}" == *"timestamp"* ]]; then
        check "Insta360 gyro CSV 行数 (${GYRO_ROWS} 行)" "pass"
    else
        check "Insta360 gyro CSV 基本检查" "行数=${GYRO_ROWS} header='${GYRO_HEADER}'"
    fi
    # 检查 timestamp 是否递增（取前 20 行）
    if awk -F, 'NR>1 && NR<=20 { if ($1 < prev) { exit 1 } prev=$1 }' "${INSTA_GYRO}"; then
        check "Insta360 gyro 时间戳单调性（前20行）" "pass"
    else
        check "Insta360 gyro 时间戳单调性" "时间戳非单调递增"
    fi
    # 检查加速度计合向量接近 1g（与安装方向无关）
    ACCEL_MAG=$(awk -F, 'NR>1 && NR<=200 {
        ax=$2; ay=$3; az=$4
        mag = sqrt(ax*ax + ay*ay + az*az)
        s+=mag; n++
    } END {if(n>0) printf "%.3f", s/n}' "${INSTA_GYRO}")
    if awk "BEGIN { v=${ACCEL_MAG}; exit !(v>0.85 && v<1.15) }"; then
        check "Insta360 加速度计合向量均值 (${ACCEL_MAG} ≈ 1g)" "pass"
    else
        check "Insta360 加速度计合向量均值" "异常值: ${ACCEL_MAG}，期望约 1.0"
    fi
else
    check "Insta360 gyro CSV 存在" "文件不存在: ${INSTA_GYRO}"
fi

# Mid360 点云 CSV
if [[ -f "${MID360_POINTS}" ]]; then
    POINTS_ROWS=$(wc -l < "${MID360_POINTS}")
    POINTS_HEADER=$(head -1 "${MID360_POINTS}")
    if [[ $POINTS_ROWS -gt 1000 ]] && [[ "${POINTS_HEADER}" == *"timestamp"* ]]; then
        check "Mid360 点云 CSV 行数 (${POINTS_ROWS} 点)" "pass"
    else
        check "Mid360 点云 CSV 基本检查" "行数=${POINTS_ROWS} header='${POINTS_HEADER}'"
    fi
else
    check "Mid360 点云 CSV 存在" "文件不存在: ${MID360_POINTS}"
fi

# Mid360 IMU CSV
if [[ -f "${MID360_IMU}" ]]; then
    IMU_ROWS=$(wc -l < "${MID360_IMU}")
    IMU_HEADER=$(head -1 "${MID360_IMU}")
    if [[ $IMU_ROWS -gt 100 ]] && [[ "${IMU_HEADER}" == *"timestamp"* ]]; then
        check "Mid360 IMU CSV 行数 (${IMU_ROWS} 行)" "pass"
    else
        check "Mid360 IMU CSV 基本检查" "行数=${IMU_ROWS} header='${IMU_HEADER}'"
    fi
    # 检查 IMU 时间戳单调性
    if awk -F, 'NR>1 && NR<=50 { if ($1 < prev) { exit 1 } prev=$1 }' "${MID360_IMU}"; then
        check "Mid360 IMU 时间戳单调性（前50行）" "pass"
    else
        check "Mid360 IMU 时间戳单调性" "时间戳非单调递增"
    fi
else
    check "Mid360 IMU CSV 存在" "文件不存在: ${MID360_IMU}"
fi

# ── 汇总 ─────────────────────────────────────────────
echo ""
echo "================================================================"
echo " 验证结果汇总"
echo "   PASS : ${PASS}"
echo "   FAIL : ${FAIL}"
if [[ $FAIL -eq 0 ]]; then
    echo "   结论 : ✓ 数据验证通过"
else
    echo "   结论 : ✗ 存在异常，请检查上方 FAIL 条目"
fi
echo ""
echo " 提取文件："
ls -lh "${OUT_DIR}"/ 2>/dev/null
echo "================================================================"

[[ $FAIL -eq 0 ]]
