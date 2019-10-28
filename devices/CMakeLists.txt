cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME devices)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

set( Boost_USE_STATIC_LIBS ON ) # Static Link Libraries ( libboost_* )
set( Boost_USE_MULTITHREADED ON ) # Multi Thread Libraries ( *-mt-* )
set( Boost_USE_STATIC_RUNTIME OFF ) # Static Runtime Libraries ( *-s* )
set( Boost_INCLUDE_DIR /usr/include)
set( Boost_LIBRARY_DIR /usr/lib/x86_64-linux-gnu)

set(FLY_CAPTURE_INCLUDE_DIR /usr/include)
set(FLT_CAPTURE_LIBS /usr/lib/libflycapture.so) 

set(JPEG_TURBO_ROOT_DIR /opt/libjpeg-turbo)
set(JPEG_TURBO_INCLUDE_DIR ${JPEG_TURBO_ROOT_DIR}/include)
set(JPEG_TURBO_LIBRARY_DIR ${JPEG_TURBO_ROOT_DIR}/lib64)
set(JPEG_TURBO_LIBS
    ${JPEG_TURBO_LIBRARY_DIR}/libjpeg.so
    ${JPEG_TURBO_LIBRARY_DIR}/libturbojpeg.so
)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
)
include_directories(
    "../"
    "/usr/local/include/"
    ${Boost_INCLUDE_DIR}
    ${FLY_CAPTURE_INCLUDE_DIR}
    ${JPEG_TURBO_INCLUDE_DIR}
)
add_library(${PROJECT_NAME}
    ${ALL_SRC}
)
target_link_libraries(${PROJECT_NAME}
    common
    Threads::Threads
    boost_system
    ${FLT_CAPTURE_LIBS}
    ${JPEG_TURBO_LIBS}
)