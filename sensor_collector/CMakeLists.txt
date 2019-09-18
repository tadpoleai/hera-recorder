cmake_minimum_required( VERSION 2.8 )

# Require C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Werror -Wall -Wextra")
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(PROJECT_NAME sensor_collector)
project(${PROJECT_NAME})

find_package(Threads REQUIRED)

set( Boost_USE_STATIC_LIBS ON ) # Static Link Libraries ( libboost_* )
set( Boost_USE_MULTITHREADED ON ) # Multi Thread Libraries ( *-mt-* )
set( Boost_USE_STATIC_RUNTIME OFF ) # Static Runtime Libraries ( *-s* )
set( Boost_INCLUDE_DIR /usr/include)    
set( Boost_LIBRARY_DIR /usr/lib/x86_64-linux-gnu)

file(GLOB_RECURSE
    ALL_SRC
    "src/**/*.cpp"
    "../common/**/*.cpp"
)
include_directories(
    "../"
    ${Boost_INCLUDE_DIR} 
)

add_executable(sensor_test
    test/sensor_test.cpp
    ${ALL_SRC}
)
target_link_libraries(sensor_test
    Threads::Threads
    boost_system
)

#add_executable(sensor_list_factory_test
#    test/sensor_list_factory_test.cpp
#    ${ALL_SRC}
#)
#target_link_libraries(sensor_list_factory_test
#    Threads::Threads
#)
