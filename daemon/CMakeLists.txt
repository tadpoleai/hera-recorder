cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME hera-daemon)
project(${PROJECT_NAME})

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
)
include_directories(
    "../"
    "/usr/local/include/"
    ${CMAKE_BINARY_DIR}
)
add_executable(${PROJECT_NAME}
    ${ALL_SRC}
)
target_link_libraries(${PROJECT_NAME}
    hera-common
    hera-devices
    "/usr/local/lib/libthrift-0.12.0.so"
)

# install(
#     FILES script/hera-daemon
#     PERMISSIONS OWNER_EXECUTE OWNER_READ
#     DESTINATION /etc/init.d
# )
# install(
#     FILES script/hera-daemon.service
#     PERMISSIONS OWNER_EXECUTE OWNER_READ
#     DESTINATION /lib/systemd/system
# )
# install(
#     TARGETS hera-daemon
#     PERMISSIONS OWNER_EXECUTE OWNER_READ
#     DESTINATION /usr/local/bin
# )
