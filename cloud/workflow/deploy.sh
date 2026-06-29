#!/bin/bash
# =============================================================================
# 一键部署脚本：构建 Docker 镜像、推送到 ACR、部署 FC 函数、创建云工作流
#
# 使用前需配置以下环境变量（或直接修改下方 CONFIG 区域）：
#   ALIYUN_REGION       - 地域，如 cn-hangzhou
#   ALIYUN_ACCOUNT_ID   - 阿里云账号 ID（数字）
#   ACR_REGISTRY        - ACR 实例地址，如 registry.cn-hangzhou.aliyuncs.com
#   ACR_NAMESPACE       - ACR 命名空间
#   OSS_BUCKET          - 数据 OSS bucket 名
#   RAM_ROLE_ARN        - FC 服务绑定的 RAM 角色 ARN（需有 OSS 读写权限）
# =============================================================================
set -euo pipefail

# ---- CONFIG -----------------------------------------------------------------
REGION="${ALIYUN_REGION:-cn-hangzhou}"
ACCOUNT_ID="${ALIYUN_ACCOUNT_ID:?请设置 ALIYUN_ACCOUNT_ID 环境变量}"
ACR_REGISTRY="${ACR_REGISTRY:-registry.${REGION}.aliyuncs.com}"
ACR_NAMESPACE="${ACR_NAMESPACE:-hera}"
IMAGE_TAG="${IMAGE_TAG:-latest}"

# hera-stitch：双鱼眼拼接服务
STITCH_IMAGE_NAME="hera-stitch"
STITCH_IMAGE="${ACR_REGISTRY}/${ACR_NAMESPACE}/${STITCH_IMAGE_NAME}:${IMAGE_TAG}"
FC_STITCH_SERVICE="hera-stitch-svc"
FC_STITCH_FUNCTION="hera-stitch"

# hera-convert：hera → ROS bag 转换服务
CONVERT_IMAGE_NAME="hera-convert"
CONVERT_IMAGE="${ACR_REGISTRY}/${ACR_NAMESPACE}/${CONVERT_IMAGE_NAME}:${IMAGE_TAG}"
FC_CONVERT_SERVICE="hera-convert-svc"
FC_CONVERT_FUNCTION="hera-convert"

RAM_ROLE_ARN="${RAM_ROLE_ARN:?请设置 RAM_ROLE_ARN 环境变量}"
WORKFLOW_NAME="hera-to-rosbag"

# 脚本所在目录（recorder 根目录）
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# -----------------------------------------------------------------------------

log() { echo "[$(date '+%H:%M:%S')] $*"; }

# ---------------------------------------------------------------------------
# 1. 构建 Docker 镜像
# ---------------------------------------------------------------------------
build_stitch_image() {
    log "Building hera-stitch image: ${STITCH_IMAGE}"
    docker build \
        -f "${REPO_ROOT}/cloud/docker/Dockerfile.stitch" \
        -t "${STITCH_IMAGE}" \
        "${REPO_ROOT}"
    log "hera-stitch build complete."
}

build_convert_image() {
    log "Building hera-convert image: ${CONVERT_IMAGE}"
    docker build \
        -f "${REPO_ROOT}/cloud/docker/Dockerfile" \
        -t "${CONVERT_IMAGE}" \
        "${REPO_ROOT}"
    log "hera-convert build complete."
}

build_image() {
    build_stitch_image
    build_convert_image
}

# ---------------------------------------------------------------------------
# 2. 推送镜像到 ACR
# ---------------------------------------------------------------------------
push_image() {
    log "Pushing images to ACR: ${ACR_REGISTRY}"
    docker push "${STITCH_IMAGE}"
    docker push "${CONVERT_IMAGE}"
    log "Push complete."
}

# ---------------------------------------------------------------------------
# 3a. 部署 hera-stitch FC 函数
# ---------------------------------------------------------------------------
deploy_fc_stitch() {
    log "Deploying FC service: ${FC_STITCH_SERVICE}"
    aliyun fc CreateService --region "${REGION}" \
        --ServiceName "${FC_STITCH_SERVICE}" \
        --Role "${RAM_ROLE_ARN}" \
        --Description "hera stitching service" \
        --InternetAccess true 2>/dev/null || \
    aliyun fc UpdateService --region "${REGION}" \
        --ServiceName "${FC_STITCH_SERVICE}" \
        --Role "${RAM_ROLE_ARN}"

    FUNCTION_JSON=$(cat <<EOF
{
  "FunctionName": "${FC_STITCH_FUNCTION}",
  "Description": "Dual-fisheye stitching: raw .hera -> stitched .hera",
  "Runtime": "custom-container",
  "Timeout": 86400,
  "MemorySize": 4096,
  "CAPort": 9001,
  "CustomContainerConfig": {
    "Image": "${STITCH_IMAGE}",
    "Command": "[\"python3\", \"/opt/stitch_handler.py\"]",
    "AccelerationType": "Default"
  },
  "EnvironmentVariables": {
    "HERA_DEVICE_PLUGIN_PATH": "/usr/local/lib/hera/plugin",
    "HERA_DEVICE_LOAD_DRIVER": "0"
  }
}
EOF
)
    echo "${FUNCTION_JSON}" | \
        aliyun fc CreateFunction --region "${REGION}" \
            --ServiceName "${FC_STITCH_SERVICE}" \
            --stdin 2>/dev/null || \
    echo "${FUNCTION_JSON}" | \
        aliyun fc UpdateFunction --region "${REGION}" \
            --ServiceName "${FC_STITCH_SERVICE}" \
            --FunctionName "${FC_STITCH_FUNCTION}" \
            --stdin
    log "hera-stitch FC deployment complete."
}

# ---------------------------------------------------------------------------
# 3b. 部署 hera-convert FC 函数
# ---------------------------------------------------------------------------
deploy_fc_convert() {
    log "Deploying FC service: ${FC_CONVERT_SERVICE}"
    aliyun fc CreateService --region "${REGION}" \
        --ServiceName "${FC_CONVERT_SERVICE}" \
        --Role "${RAM_ROLE_ARN}" \
        --Description "hera-convert cloud service" \
        --InternetAccess true 2>/dev/null || \
    aliyun fc UpdateService --region "${REGION}" \
        --ServiceName "${FC_CONVERT_SERVICE}" \
        --Role "${RAM_ROLE_ARN}"

    FUNCTION_JSON=$(cat <<EOF
{
  "FunctionName": "${FC_CONVERT_FUNCTION}",
  "Description": "Convert stitched .hera to ROS bag",
  "Runtime": "custom-container",
  "Timeout": 86400,
  "MemorySize": 4096,
  "CAPort": 9000,
  "CustomContainerConfig": {
    "Image": "${CONVERT_IMAGE}",
    "Command": "[\"python3\", \"/opt/fc_handler.py\"]",
    "AccelerationType": "Default"
  },
  "EnvironmentVariables": {
    "HERA_DEVICE_PLUGIN_PATH": "/usr/local/lib/hera/plugin",
    "HERA_DEVICE_LOAD_DRIVER": "0"
  }
}
EOF
)
    echo "${FUNCTION_JSON}" | \
        aliyun fc CreateFunction --region "${REGION}" \
            --ServiceName "${FC_CONVERT_SERVICE}" \
            --stdin 2>/dev/null || \
    echo "${FUNCTION_JSON}" | \
        aliyun fc UpdateFunction --region "${REGION}" \
            --ServiceName "${FC_CONVERT_SERVICE}" \
            --FunctionName "${FC_CONVERT_FUNCTION}" \
            --stdin
    log "hera-convert FC deployment complete."
}

deploy_fc() {
    deploy_fc_stitch
    deploy_fc_convert
}

# ---------------------------------------------------------------------------
# 4. 创建 / 更新 Serverless Workflow (云工作流)
# ---------------------------------------------------------------------------
deploy_workflow() {
    log "Deploying Serverless Workflow: ${WORKFLOW_NAME}"

    # 将 workflow YAML 中的占位符替换为实际值
    DEFINITION=$(sed \
        -e "s|{region}|${REGION}|g" \
        -e "s|{account-id}|${ACCOUNT_ID}|g" \
        -e "s|{fc-stitch-service}|${FC_STITCH_SERVICE}|g" \
        -e "s|{fc-stitch-function}|${FC_STITCH_FUNCTION}|g" \
        -e "s|{fc-convert-service}|${FC_CONVERT_SERVICE}|g" \
        -e "s|{fc-convert-function}|${FC_CONVERT_FUNCTION}|g" \
        "${REPO_ROOT}/cloud/workflow/convert_workflow.yaml")

    ESCAPED=$(echo "${DEFINITION}" | python3 -c "import sys,json; print(json.dumps(sys.stdin.read()))")

    aliyun fnf CreateFlow --region "${REGION}" \
        --Name "${WORKFLOW_NAME}" \
        --Type FDL \
        --Definition "${ESCAPED}" \
        --Description "Convert Hera data to ROS bag via FC" 2>/dev/null || \
    aliyun fnf UpdateFlow --region "${REGION}" \
        --Name "${WORKFLOW_NAME}" \
        --Definition "${ESCAPED}"

    log "Workflow deployment complete."
    log "Workflow ARN: acs:fnf:${REGION}:${ACCOUNT_ID}:flow/${WORKFLOW_NAME}"
}

# ---------------------------------------------------------------------------
# 5. 打印触发示例
# ---------------------------------------------------------------------------
print_usage() {
    cat <<EOF

=== 部署完成 ===
触发云工作流（示例）：

aliyun fnf StartExecution --region ${REGION} \\
  --FlowName ${WORKFLOW_NAME} \\
  --ExecutionName "run-$(date +%Y%m%d%H%M%S)" \\
  --Input '{
    "oss_endpoint":        "oss-${REGION}-internal.aliyuncs.com",
    "oss_bucket":          "<your-bucket>",
    "src_oss_prefix":      "hera-data/session-20240101/",
    "stitched_oss_prefix": "hera-stitched/session-20240101/",
    "dst_oss_path":        "rosbag/session-20240101.bag",
    "mode":                "auto",
    "fps":                 1
  }'

本地运行拼接服务（无 OSS，用于调试）：
  docker run --rm -p 9001:9001 ${STITCH_IMAGE}
  curl -X POST http://localhost:9001/invoke -d '{"oss_endpoint":...}'

EOF
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
CMD="${1:-all}"
case "${CMD}" in
    build)           build_image ;;
    build-stitch)    build_stitch_image ;;
    build-convert)   build_convert_image ;;
    push)            push_image ;;
    fc)              deploy_fc ;;
    fc-stitch)       deploy_fc_stitch ;;
    fc-convert)      deploy_fc_convert ;;
    workflow)        deploy_workflow ;;
    all)
        build_image
        push_image
        deploy_fc
        deploy_workflow
        print_usage
        ;;
    *)
        echo "Usage: $0 [build|push|fc|workflow|all]"
        exit 1
        ;;
esac
