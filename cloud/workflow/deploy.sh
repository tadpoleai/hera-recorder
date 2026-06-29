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
IMAGE_NAME="hera-convert"
IMAGE_TAG="${IMAGE_TAG:-latest}"
FULL_IMAGE="${ACR_REGISTRY}/${ACR_NAMESPACE}/${IMAGE_NAME}:${IMAGE_TAG}"

FC_SERVICE_NAME="hera-convert-svc"
FC_FUNCTION_NAME="hera-convert"
RAM_ROLE_ARN="${RAM_ROLE_ARN:?请设置 RAM_ROLE_ARN 环境变量}"

WORKFLOW_NAME="hera-to-rosbag"

# 脚本所在目录（recorder 根目录）
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
# -----------------------------------------------------------------------------

log() { echo "[$(date '+%H:%M:%S')] $*"; }

# ---------------------------------------------------------------------------
# 1. 构建 Docker 镜像
# ---------------------------------------------------------------------------
build_image() {
    log "Building Docker image: ${FULL_IMAGE}"
    docker build \
        -f "${REPO_ROOT}/cloud/docker/Dockerfile" \
        -t "${FULL_IMAGE}" \
        "${REPO_ROOT}"
    log "Build complete."
}

# ---------------------------------------------------------------------------
# 2. 推送镜像到 ACR
# ---------------------------------------------------------------------------
push_image() {
    log "Logging in to ACR: ${ACR_REGISTRY}"
    # 使用 aliyun CLI 获取临时密码，或由用户预先 docker login
    # docker login --username="${ACR_USER}" --password="${ACR_PASS}" "${ACR_REGISTRY}"
    log "Pushing image: ${FULL_IMAGE}"
    docker push "${FULL_IMAGE}"
    log "Push complete."
}

# ---------------------------------------------------------------------------
# 3. 创建 / 更新 FC 服务和函数
#    依赖：阿里云 CLI (aliyun) 已安装并配置好 credentials
# ---------------------------------------------------------------------------
deploy_fc() {
    log "Deploying FC service: ${FC_SERVICE_NAME}"
    aliyun fc CreateService --region "${REGION}" \
        --ServiceName "${FC_SERVICE_NAME}" \
        --Role "${RAM_ROLE_ARN}" \
        --Description "hera-convert cloud service" \
        --InternetAccess true 2>/dev/null || \
    aliyun fc UpdateService --region "${REGION}" \
        --ServiceName "${FC_SERVICE_NAME}" \
        --Role "${RAM_ROLE_ARN}"

    log "Deploying FC function: ${FC_FUNCTION_NAME}"
    FUNCTION_JSON=$(cat <<EOF
{
  "FunctionName": "${FC_FUNCTION_NAME}",
  "Description": "Convert Hera data to ROS bag",
  "Runtime": "custom-container",
  "Timeout": 86400,
  "MemorySize": 4096,
  "CAPort": 9000,
  "CustomContainerConfig": {
    "Image": "${FULL_IMAGE}",
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
    # Try create first; fall back to update if function already exists
    echo "${FUNCTION_JSON}" | \
        aliyun fc CreateFunction --region "${REGION}" \
            --ServiceName "${FC_SERVICE_NAME}" \
            --stdin 2>/dev/null || \
    echo "${FUNCTION_JSON}" | \
        aliyun fc UpdateFunction --region "${REGION}" \
            --ServiceName "${FC_SERVICE_NAME}" \
            --FunctionName "${FC_FUNCTION_NAME}" \
            --stdin

    log "FC deployment complete."
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
        -e "s|{fc-service-name}|${FC_SERVICE_NAME}|g" \
        -e "s|{fc-function-name}|${FC_FUNCTION_NAME}|g" \
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
    "oss_endpoint":   "oss-${REGION}-internal.aliyuncs.com",
    "oss_bucket":     "<your-bucket>",
    "src_oss_prefix": "hera-data/session-20240101/",
    "dst_oss_path":   "rosbag/session-20240101.bag"
  }'

EOF
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
CMD="${1:-all}"
case "${CMD}" in
    build)    build_image ;;
    push)     push_image ;;
    fc)       deploy_fc ;;
    workflow) deploy_workflow ;;
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
