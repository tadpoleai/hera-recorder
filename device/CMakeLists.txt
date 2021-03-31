cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-device)
project(${PROJECT_NAME})

include("../cmake/include/cpp_common.cmake")
include("../cmake/include/turbojpeg.cmake")

find_package(Threads REQUIRED)

# Find source files
file(GLOB_RECURSE BASE_SRC "base/*.cpp")
file(GLOB_RECURSE DISPLAY_SRC "display/*.cpp")
file(GLOB_RECURSE DRIVERS_SRC "driver/*.cpp")
file(GLOB_RECURSE PLUGIN_SRC_LIST "plugin/**/plugin_entry.cpp")

# Set include
include_directories(. include .. plugin ${CMAKE_BINARY_DIR}
                    ${JPEG_TURBO_INCLUDE_DIR})

# Build device (core)
add_library(${PROJECT_NAME} SHARED ${BASE_SRC} ${DRIVERS_SRC} ${DISPLAY_SRC})
target_link_libraries(${PROJECT_NAME} hera-common hera-storage Threads::Threads
                      ${JPEG_TURBO_LIBS} ${CMAKE_DL_LIBS})

# Build parameter definition generator
set(GENERATOR tmp-hera-parameter-generator)
add_executable(${GENERATOR} generator/parser.cpp generator/output.cpp
                            generator/main.cpp)

# Define every driver plugin
foreach(PLUGIN_SRC ${PLUGIN_SRC_LIST})
  # Regex to get plugin name
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\1" PLUGIN_CATEGORY_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\2" PLUGIN_VENDOR_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE "plugin_entry\\.cpp" "" PLUGIN_DIR ${PLUGIN_SRC})
  set(PLUGIN_NAME ${PLUGIN_CATEGORY_NAME}-${PLUGIN_VENDOR_NAME})
  set(PLUGIN_BASE_LIB_NAME
      ${PROJECT_NAME}-plugin-${PLUGIN_NAME}-${ARCHITECTURE}-base)
  set(PLUGIN_DRIVER_LIB_NAME
      ${PROJECT_NAME}-plugin-${PLUGIN_NAME}-${ARCHITECTURE}-driver)

  # Message
  message(
    STATUS "Hera device: find plugin ${Escape}[34m${PLUGIN_NAME}${Escape}[m")

  # Find extra cmake file of each device
  unset(PLUGIN_DRIVER_FOUND)
  unset(PLUGIN_DRIVER_DEP_INCLUDE)
  unset(PLUGIN_DRIVER_DEP_LIBS)
  set(PLUGIN_CMAKE_FILE ${PLUGIN_DIR}/plugin.cmake)
  if(EXISTS ${PLUGIN_CMAKE_FILE})
    # Define message_plugin
    function(message_plugin)
      message(STATUS "\t${PLUGIN_NAME}: " "${Escape}[33m" ${ARGV} "${Escape}[m")
    endfunction()

    # Include cmake file
    message(
      STATUS
        "\tplugin ${PLUGIN_NAME} provided standalone cmake file, invoking it")
    include(${PLUGIN_CMAKE_FILE})

    if(PLUGIN_DRIVER_FOUND)
      message(STATUS "\t${Escape}[32m"
                     "plugin ${PLUGIN_NAME}'s driver is buildable"
                     "${Escape}[m")
    else()
      message(STATUS "\t${Escape}[31m"
                     "plugin ${PLUGIN_NAME}'s driver is not buildable"
                     "${Escape}[m")
    endif()
  else()
    # Plugin do not need extra cmake file
    set(PLUGIN_DRIVER_FOUND 1)
  endif()

  # Call parameter definition
  if(NOT DEFINED ENV{CROSS_COMPILE} AND NOT DEFINED CROSS_COMPILE)
    set(PLUGIN_PARAMETER_DEF ${PLUGIN_DIR}/plugin_param.src)
    set(PLUGIN_PARAMETER_GEN ${PLUGIN_DIR}/plugin_param.hpp)
    add_custom_command(
      OUTPUT ${PLUGIN_PARAMETER_GEN}
      COMMAND ${GENERATOR} ${PLUGIN_PARAMETER_DEF} ${PLUGIN_PARAMETER_GEN}
              ${PLUGIN_CATEGORY_NAME} ${PLUGIN_VENDOR_NAME}
      DEPENDS ${GENERATOR} ${PLUGIN_PARAMETER_DEF})
  endif()

  # Add dynamic library without device driver
  add_library(${PLUGIN_BASE_LIB_NAME} SHARED ${PLUGIN_SRC}
                                             ${PLUGIN_PARAMETER_GEN})
  target_link_libraries(${PLUGIN_BASE_LIB_NAME} ${PROJECT_NAME})
  set_target_properties(
    ${PLUGIN_BASE_LIB_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                       ${CMAKE_CURRENT_BINARY_DIR}/base)

  # Install plugin
  install(
    TARGETS ${PLUGIN_BASE_LIB_NAME}
    PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
                WORLD_READ
    DESTINATION lib/hera/plugin/base)

  # Add dynamic library with device driver
  if(PLUGIN_DRIVER_FOUND)
    add_library(${PLUGIN_DRIVER_LIB_NAME} SHARED ${PLUGIN_SRC}
                                                 ${PLUGIN_PARAMETER_GEN})
    target_include_directories(${PLUGIN_DRIVER_LIB_NAME}
                               PRIVATE ${PLUGIN_DRIVER_DEP_INCLUDE})
    target_link_libraries(${PLUGIN_DRIVER_LIB_NAME}
                          PRIVATE ${PLUGIN_DRIVER_DEP_LIBS})
    target_compile_definitions(${PLUGIN_DRIVER_LIB_NAME} PRIVATE WITH_DRIVER=1)
    set_target_properties(
      ${PLUGIN_DRIVER_LIB_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                           ${CMAKE_CURRENT_BINARY_DIR}/driver)

    # Install plugin
    install(
      TARGETS ${PLUGIN_DRIVER_LIB_NAME}
      PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
                  WORLD_EXECUTE WORLD_READ
      DESTINATION lib/hera/plugin/driver)
  endif()

endforeach(PLUGIN_SRC)

if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME} hera-git-info)
endif()

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/device/)

# Build test
add_executable(${PROJECT_NAME}-test src/dummy_test.cpp)
target_link_libraries(${PROJECT_NAME}-test hera-common hera-storage
                      ${PROJECT_NAME})

# Build record
add_executable(${PROJECT_NAME}-record src/record.cpp)
target_link_libraries(${PROJECT_NAME}-record hera-common hera-storage
                      ${PROJECT_NAME})
