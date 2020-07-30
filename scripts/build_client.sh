echo "Building client"

cd $(dirname "$0")/..

mkdir -p build_client

cd client
npm install
npm run thrift
npm run build -- --dest ../build_client
