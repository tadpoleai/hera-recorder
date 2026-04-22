cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-storage)
project(${PROJECT_NAME})

include("../cmake/include/cpp_common.cmake")

find_package(Threads REQUIRED)
include_directories(include .. ${CMAKE_BINARY_DIR})

file(GLOB BASE_SRC "src/*.cpp")
file(GLOB_RECURSE PLUGIN_SRC_LIST "src/upload/**/plugin_entry.cpp")

# Set include
include_directories(include .. ${CMAKE_BINARY_DIR})

# Build storage (core)
add_library(${PROJECT_NAME} SHARED ${BASE_SRC})
target_link_libraries(${PROJECT_NAME} hera-common Threads::Threads
                      ${CMAKE_DL_LIBS})
if(GIT_INFO_ENABLED)
  target_link_libraries(${PROJECT_NAME} hera-git-info)
endif()

# Define every upload plugin
foreach(PLUGIN_SRC ${PLUGIN_SRC_LIST})
  # Regex to get plugin name
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\1" PLUGIN_CATEGORY_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\2" PLUGIN_VENDOR_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE "plugin_entry\\.cpp" "" PLUGIN_DIR ${PLUGIN_SRC})
  set(PLUGIN_NAME ${PLUGIN_CATEGORY_NAME}-${PLUGIN_VENDOR_NAME})
  set(PLUGIN_LIB_NAME ${PROJECT_NAME}-${PLUGIN_NAME}-${ARCHITECTURE})

  # Message
  message(
    STATUS
      "Hera storage: find upload plugin ${Escape}[34m${PLUGIN_NAME}${Escape}[m")

  # Find extra cmake file of each device
  unset(PLUGIN_DEP_FOUND)
  unset(PLUGIN_DEP_INCLUDE)
  unset(PLUGIN_DEP_LIBS)
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

    if(PLUGIN_DEP_FOUND)
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
    set(PLUGIN_DEP_FOUND 1)
  endif()

  if(PLUGIN_DEP_FOUND)
    add_library(${PLUGIN_LIB_NAME} SHARED ${PLUGIN_SRC})
    target_include_directories(${PLUGIN_LIB_NAME} PRIVATE ${PLUGIN_DEP_INCLUDE})
    target_link_libraries(${PLUGIN_LIB_NAME} PRIVATE ${PLUGIN_DEP_LIBS})
    set_target_properties(
      ${PLUGIN_LIB_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                    ${CMAKE_CURRENT_BINARY_DIR}/upload)

    # Install plugin
    install(
      TARGETS ${PLUGIN_LIB_NAME}
      PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ
                  WORLD_EXECUTE WORLD_READ
      DESTINATION lib/hera/plugin/upload)
  endif()

endforeach(PLUGIN_SRC)

file(GLOB_RECURSE TOOL_SRC "tool/*.cpp")
# Separate extract_insta from main tool
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/extract_insta.cpp")
add_executable(${PROJECT_NAME}-tool ${TOOL_SRC})
target_link_libraries(${PROJECT_NAME}-tool hera-common hera-storage hera-device)

# Build extract-insta same way as tool
add_executable(${PROJECT_NAME}-extract-insta tool/extract_insta.cpp)
target_link_libraries(${PROJECT_NAME}-extract-insta hera-common hera-storage hera-device ${CMAKE_DL_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  # Keep transitive shared-lib dependencies during link (avoids dropping hera-device with --as-needed)
  target_link_options(${PROJECT_NAME}-extract-insta PRIVATE -Wl,--no-as-needed)
endif()

# Install shared libraries
install(
  TARGETS ${PROJECT_NAME}
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION lib)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include/hera/storage/)

# Install tools
install(
  TARGETS ${PROJECT_NAME}-tool ${PROJECT_NAME}-extract-insta
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
