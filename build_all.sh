set -e

cd client
npm i
npm run thrift
npm run build
if [ $? -ne 0 ];then
        echo "client error"
fi

cd ..
mkdir build
cd build
cmake ..
make -j
if [ $? -ne 0 ];then
        echo "build error"
fi

tar -czvf install.tar.gz  convert/hera-convert daemon/hera-daemon ../daemon/script ../client/dist ../README.md ../INSTALLATION.md ../client/USAGE.md ../convert/USAGE.md ../deploy/*.md ../jenkins_deploy.sh
