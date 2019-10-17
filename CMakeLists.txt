cmake_minimum_required( VERSION 2.8 )

set(PROJECT_NAME tron)
project(${PROJECT_NAME})

add_subdirectory(common)
add_subdirectory(devices)
add_subdirectory(daemon)
add_subdirectory(convert)