#protoc --cpp_out=proto_cpp sensor_collector.proto
#protoc --grpc_out=proto_cpp --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin sensor_collector.proto
thrift --strict --gen cpp tron.thrift