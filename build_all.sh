echo "Building"

rm -rf artifacts

bash scripts/build_amd64.sh $1
bash scripts/build_arm_host.sh $1

tar -czvf artifacts.tar.gz artifacts