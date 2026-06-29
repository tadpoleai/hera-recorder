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
# Separate per-device tools from the main tool binary
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/extract_insta.cpp")
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/extract_mid360.cpp")
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/ingest_insta_video.cpp")
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/ingest_stream_hera.cpp")
list(REMOVE_ITEM TOOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/tool/validate_stitch_mediasdk.cpp")
add_executable(${PROJECT_NAME}-tool ${TOOL_SRC})
target_link_libraries(${PROJECT_NAME}-tool hera-common hera-storage hera-device)

# Build extract-insta
add_executable(${PROJECT_NAME}-extract-insta tool/extract_insta.cpp)
target_link_libraries(${PROJECT_NAME}-extract-insta hera-common hera-storage hera-device ${CMAKE_DL_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(${PROJECT_NAME}-extract-insta PRIVATE -Wl,--no-as-needed)
endif()

# Build extract-mid360
add_executable(${PROJECT_NAME}-extract-mid360 tool/extract_mid360.cpp)
target_link_libraries(${PROJECT_NAME}-extract-mid360 hera-common hera-storage hera-device ${CMAKE_DL_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(${PROJECT_NAME}-extract-mid360 PRIVATE -Wl,--no-as-needed)
endif()

# Build ingest-insta-video (decode RecordDownload MP4 -> InstaJpegFramePacket .hera)
add_executable(${PROJECT_NAME}-ingest-insta-video tool/ingest_insta_video.cpp)
target_compile_definitions(${PROJECT_NAME}-ingest-insta-video PRIVATE HERA_COMPILE_IN_REPO)
target_link_libraries(${PROJECT_NAME}-ingest-insta-video hera-common hera-storage hera-device ${CMAKE_DL_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(${PROJECT_NAME}-ingest-insta-video PRIVATE -Wl,--no-as-needed)
endif()

# Build ingest-stream-hera (decode Stream-mode H.264 .hera -> InstaJpegFramePacket .hera)
add_executable(${PROJECT_NAME}-ingest-stream-hera tool/ingest_stream_hera.cpp)
target_compile_definitions(${PROJECT_NAME}-ingest-stream-hera PRIVATE HERA_COMPILE_IN_REPO)
target_link_libraries(${PROJECT_NAME}-ingest-stream-hera hera-common hera-storage hera-device ${CMAKE_DL_LIBS})
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_link_options(${PROJECT_NAME}-ingest-stream-hera PRIVATE -Wl,--no-as-needed)
endif()

# Optional: MediaSDK stitching backend (amd64 only, requires libMediaSDK installed).
# Install via: sudo dpkg -i libMediaSDK-dev-*.deb
# then rebuild with: cmake --build . --target hera-storage-ingest-insta-video
#                                            hera-storage-validate-stitch-mediasdk
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
  find_path(_MEDIASDK_INC NAMES ins_stitcher.h ins_realtime_stitcher.h
            PATHS /usr /usr/local
            PATH_SUFFIXES include)
  find_library(_MEDIASDK_LIB NAMES MediaSDK
               PATHS /usr /usr/local
               PATH_SUFFIXES lib lib64)
  if(_MEDIASDK_INC AND _MEDIASDK_LIB)
    message(STATUS "MediaSDK found: ${_MEDIASDK_LIB}")

    # Wire MediaSDK into ingest-insta-video
    message(STATUS "  → ingest-insta-video will use Insta360 stitcher")
    target_compile_definitions(${PROJECT_NAME}-ingest-insta-video PRIVATE HAVE_MEDIASDK)
    target_include_directories(${PROJECT_NAME}-ingest-insta-video PRIVATE ${_MEDIASDK_INC})
    target_link_libraries(${PROJECT_NAME}-ingest-insta-video ${_MEDIASDK_LIB})

    # Validation tool: RealTimeStitcher on stored hera data
    # Prefer turbojpeg for JPEG output; fall back to PPM if not installed.
    find_library(_TURBOJPEG_LIB NAMES turbojpeg
                 PATHS /opt/libjpeg-turbo/lib64 /usr/lib/x86_64-linux-gnu /usr/lib /usr/local/lib)
    find_path(_TURBOJPEG_INC NAMES turbojpeg.h
              PATHS /opt/libjpeg-turbo/include /usr/include /usr/local/include)

    add_executable(${PROJECT_NAME}-validate-stitch-mediasdk
                   tool/validate_stitch_mediasdk.cpp)
    target_compile_definitions(${PROJECT_NAME}-validate-stitch-mediasdk
                               PRIVATE HERA_COMPILE_IN_REPO)
    target_include_directories(${PROJECT_NAME}-validate-stitch-mediasdk
                               PRIVATE ${_MEDIASDK_INC})
    target_link_libraries(${PROJECT_NAME}-validate-stitch-mediasdk
                          hera-common hera-storage hera-device
                          ${_MEDIASDK_LIB}
                          Threads::Threads
                          ${CMAKE_DL_LIBS})
    if(_TURBOJPEG_LIB AND _TURBOJPEG_INC)
      target_compile_definitions(${PROJECT_NAME}-validate-stitch-mediasdk
                                 PRIVATE HAVE_TURBOJPEG)
      target_include_directories(${PROJECT_NAME}-validate-stitch-mediasdk
                                 PRIVATE ${_TURBOJPEG_INC})
      target_link_libraries(${PROJECT_NAME}-validate-stitch-mediasdk ${_TURBOJPEG_LIB})
      message(STATUS "  → validate-stitch-mediasdk: JPEG output via turbojpeg")
    else()
      message(STATUS "  → validate-stitch-mediasdk: turbojpeg not found, will output PPM")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
      target_link_options(${PROJECT_NAME}-validate-stitch-mediasdk
                          PRIVATE -Wl,--no-as-needed)
    endif()
    message(STATUS "  → validate-stitch-mediasdk target added")
  else()
    message(STATUS "MediaSDK not found — ingest-insta-video will use ffmpeg v360 fallback")
    message(STATUS "  validate-stitch-mediasdk target skipped")
  endif()
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
  TARGETS ${PROJECT_NAME}-tool ${PROJECT_NAME}-extract-insta ${PROJECT_NAME}-extract-mid360
          ${PROJECT_NAME}-ingest-insta-video ${PROJECT_NAME}-ingest-stream-hera
  PERMISSIONS OWNER_EXECUTE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE
              WORLD_READ
  DESTINATION bin)
