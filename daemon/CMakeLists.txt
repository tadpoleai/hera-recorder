cmake_minimum_required( VERSION 3.5.1 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME daemon)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

set( Boost_USE_STATIC_LIBS ON ) # Static Link Libraries ( libboost_* )
set( Boost_USE_MULTITHREADED ON ) # Multi Thread Libraries ( *-mt-* )
set( Boost_USE_STATIC_RUNTIME OFF ) # Static Runtime Libraries ( *-s* )
set( Boost_INCLUDE_DIR /usr/include)
set( Boost_LIBRARY_DIR /usr/lib/x86_64-linux-gnu)

file(GLOB_RECURSE
    ALL_SRC
    "src/*.cpp"
    "src/**/*.cpp"
)
include_directories(
    "../"
    "/usr/local/include/"
    ${CMAKE_BINARY_DIR}
    ${Boost_INCLUDE_DIR}
)
add_executable(daemon
    ${ALL_SRC}
)
target_link_libraries(daemon
    common
    Threads::Threads
    boost_system
    "/usr/local/lib/libthrift-0.12.0.so"
)