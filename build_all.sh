echo "Building"

cd $(dirname "$0")

rm -rf artifacts
# rm -rf build_amd64
# rm -rf build_arm
# rm -rf build_client

bash scripts/build_amd64.sh $1
# bash scripts/build_arm_host.sh $1
bash scripts/build_client.sh $1

bash scripts/make_artifacts.sh

if [ -n "$1" ]; then
    tar -czvf hera-release-$1.tar.gz artifacts --transform s/artifacts/hera-release-$1/
fi
