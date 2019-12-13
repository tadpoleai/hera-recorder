cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME hera-devices)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

set(FLY_CAPTURE_INCLUDE_DIR /usr/include)
set(FLT_CAPTURE_LIBS /usr/lib/libflycapture.so) 

set(JPEG_TURBO_ROOT_DIR /opt/libjpeg-turbo)
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_ROOT_DIR}/include)
set(JPEG_TURBO_LIBRARY_DIR ${JPEG_TURBO_ROOT_DIR}/lib64)
set(JPEG_TURBO_LIBS
    ${JPEG_TURBO_LIBRARY_DIR}/libturbojpeg.so
)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
)

include_directories(
    "../"
    ${Boost_INCLUDE_DIR}
    ${FLY_CAPTURE_INCLUDE_DIR}
    ${JPEG_TURBO_INCLUDE_DIR}

)
add_library(${PROJECT_NAME}
    STATIC
    ${ALL_SRC}
)
target_link_libraries(${PROJECT_NAME}
    hera-common
    Threads::Threads
    ${FLT_CAPTURE_LIBS}
    ${JPEG_TURBO_LIBS}
)