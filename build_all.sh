echo "Building"

rm -rf artifacts

#bash scripts/build_amd64.sh
bash scripts/build_arm_host.sh

tar -czvf artifacts.tar.gz artifacts