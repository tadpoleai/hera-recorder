echo "Building"

rm -rf artifacts

bash build_amd64.sh
bash build_arm.sh

tar -czvf artifacts.tar.gz artifacts