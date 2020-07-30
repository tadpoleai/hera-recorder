cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-client)
project(${PROJECT_NAME})

include("../cmake/include/thrift.cmake")

add_custom_target(
  client
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../client/"
  COMMAND "npm" "install"
  COMMAND "npm" "run" "thrift"
  COMMAND "npm" "run" "build")

add_custom_target(
  client-install
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../client/"
  COMMAND "npm" "install"
  COMMAND "npm" "run" "thrift"
  COMMAND "npm" "run" "build"
  COMMAND "npm" "run" "deploy-linux")
