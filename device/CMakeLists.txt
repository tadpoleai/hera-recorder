cmake_minimum_required(VERSION 3.5.1)

set(PROJECT_NAME hera-device)
project(${PROJECT_NAME})

include("../cmake/include/cpp_common.cmake")
include("../cmake/include/turbojpeg.cmake")

find_package(Threads REQUIRED)
set(CMAKE_CXX_FLAGS "-Wno-array-bounds")

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

unset(PLUGIN_REG_NAMES)
unset(PLUGIN_TYPE_VALS)
unset(PLUGIN_TYPE_NAME_PAIRS)
unset(DATA_REG_NAMES)
unset(DATA_TYPE_VALS)
unset(DATA_TYPE_NAME_PAIRS)

# Define every driver plugin
foreach(PLUGIN_SRC ${PLUGIN_SRC_LIST})
  # Regex to get plugin name
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\1" PLUGIN_CATEGORY_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE ".+/(.+)/(.+)/(.+)\\..*" "\\2" PLUGIN_VENDOR_NAME
                       ${PLUGIN_SRC})
  string(REGEX REPLACE "plugin_entry\\.cpp" "" PLUGIN_DIR ${PLUGIN_SRC})
  string(REGEX REPLACE "plugin_entry\\.cpp" "plugin_data.hpp"
                       PLUGIN_DATA_HEADER ${PLUGIN_SRC})

  set(PLUGIN_NAME ${PLUGIN_CATEGORY_NAME}-${PLUGIN_VENDOR_NAME})
  set(PLUGIN_REG_NAME_COMPOSED ${PLUGIN_CATEGORY_NAME}/${PLUGIN_VENDOR_NAME})
  set(PLUGIN_BASE_LIB_NAME
      ${PROJECT_NAME}-plugin-${PLUGIN_NAME}-${ARCHITECTURE}-base)
  set(PLUGIN_DRIVER_LIB_NAME
      ${PROJECT_NAME}-plugin-${PLUGIN_NAME}-${ARCHITECTURE}-driver)

  # Message
  message(
    STATUS "Hera device: find plugin ${Escape}[34m${PLUGIN_NAME}${Escape}[m")

  # Check device type registery
  set(HERA_PLUGIN_REG
      "HERA_PLUGIN_DEFINE_START\\(\"([a-z][0-9a-z]*/[a-z][0-9a-z]*)\", ?0x([0-9A-F]+), ?([0-9]+)\\)"
  )
  set(HERA_PLUGIN_DATA_REG
      "HERA_PLUGIN_DATA_DEFINE_START\\(([a-zA-Z][0-9a-zA-Z]*), ?0x([0-9A-F]+)\\)"
  )
  file(STRINGS "${PLUGIN_SRC}" TYPE_REG_LINE REGEX ${HERA_PLUGIN_REG})
  file(STRINGS "${PLUGIN_DATA_HEADER}" DATA_TYPE_REG_LINES
       REGEX ${HERA_PLUGIN_DATA_REG})

  if(TYPE_REG_LINE)
    # Check plugin duplication
    string(REGEX MATCH ${HERA_PLUGIN_REG} TYPE_REG_LINE_MATCH ${TYPE_REG_LINE})
    set(PLUGIN_REG_NAME ${CMAKE_MATCH_1})
    set(PLUGIN_TYPE_VAL ${CMAKE_MATCH_2})
    if(NOT ${PLUGIN_REG_NAME} STREQUAL ${PLUGIN_REG_NAME_COMPOSED})
      message(
        SEND_ERROR "\t${PLUGIN_NAME}: ${PLUGIN_SRC} \n" "${Escape}[31m"
                   "Plugin registery with different name = ${PLUGIN_REG_NAME}"
                   "${Escape}[m")
    endif()

    if(${PLUGIN_REG_NAME} IN_LIST PLUGIN_REG_NAMES)
      message(
        FATAL_ERROR
          "\t${PLUGIN_NAME}: ${PLUGIN_SRC} \n" "${Escape}[31m"
          "Plugin registery name = ${PLUGIN_REG_NAME} duplicated" "${Escape}[m")
    endif()
    if(${PLUGIN_TYPE_VAL} IN_LIST PLUGIN_TYPE_VALS)
      message(
        FATAL_ERROR
          "\t${PLUGIN_NAME}: ${PLUGIN_SRC} \n" "${Escape}[31m"
          "Plugin registery type value = ${PLUGIN_TYPE_VAL} duplicated"
          "${Escape}[m")
    endif()
    list(APPEND PLUGIN_REG_NAMES ${PLUGIN_REG_NAME})
    list(APPEND PLUGIN_TYPE_VALS ${PLUGIN_TYPE_VAL})
    list(APPEND PLUGIN_TYPE_NAME_PAIRS "${PLUGIN_TYPE_VAL}\t${PLUGIN_REG_NAME}")

    message(
      STATUS
        "\t${PLUGIN_NAME}: "
        "${Escape}[33m"
        "Factory registered plugin with name = ${PLUGIN_REG_NAME}, type value = ${PLUGIN_TYPE_VAL}"
        "${Escape}[m")

    # Check plugin data duplication
    foreach(DATA_TYPE_REG_LINE ${DATA_TYPE_REG_LINES})
      string(REGEX MATCH ${HERA_PLUGIN_DATA_REG} DATA_TYPE_REG_LINE_MATCH
                   ${DATA_TYPE_REG_LINE})
      set(DATA_REG_NAME ${CMAKE_MATCH_1})
      set(DATA_TYPE_VAL ${CMAKE_MATCH_2})
      if(${DATA_REG_NAME} IN_LIST DATA_REG_NAMES)
        message(
          FATAL_ERROR
            "\t${PLUGIN_NAME}: ${PLUGIN_DATA_HEADER} \n" "${Escape}[31m"
            "Plugin data registery name = ${DATA_REG_NAME} duplicated"
            "${Escape}[m")
      endif()
      if(${DATA_TYPE_VAL} IN_LIST DATA_TYPE_VALS)
        message(
          FATAL_ERROR
            "\t${PLUGIN_NAME}: ${PLUGIN_DATA_HEADER} \n" "${Escape}[31m"
            "Plugin data registery type value = ${DATA_TYPE_VAL} duplicated"
            "${Escape}[m")
      endif()
      list(APPEND DATA_REG_NAMES ${DATA_REG_NAME})
      list(APPEND DATA_TYPE_VALS ${DATA_TYPE_VAL})
      list(APPEND DATA_TYPE_NAME_PAIRS "${DATA_TYPE_VAL}\t${PLUGIN_REG_NAME}\t${DATA_REG_NAME}")

      message(
        STATUS
          "\t${PLUGIN_NAME}: "
          "${Escape}[33m"
          "Factory registered data with name = ${DATA_REG_NAME}, type value = ${PLUGIN_TYPE_VAL}:${DATA_TYPE_VAL}"
          "${Escape}[m")
    endforeach()
  else()
    message(
      SEND_ERROR "\t${PLUGIN_NAME}: ${PLUGIN_SRC} \n" "${Escape}[31m"
                 "Plugin entry does not contain type registery or format error"
                 "${Escape}[m")
  endif()

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
  add_library(
    ${PLUGIN_BASE_LIB_NAME} SHARED ${PLUGIN_SRC} ${PLUGIN_PARAMETER_GEN}
                                   ${PLUGIN_CONVERT_PARAMETER_GEN})
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
    add_library(
      ${PLUGIN_DRIVER_LIB_NAME} SHARED ${PLUGIN_SRC} ${PLUGIN_PARAMETER_GEN}
                                       ${PLUGIN_CONVERT_PARAMETER_GEN})
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

list(SORT PLUGIN_TYPE_NAME_PAIRS)
list(SORT DATA_TYPE_NAME_PAIRS)
message(STATUS "Hera device: Listing registered plugins")
foreach(PLUGIN_TYPE_NAME_PAIR ${PLUGIN_TYPE_NAME_PAIRS})
  message(STATUS "\t- ${Escape}[34m" "${PLUGIN_TYPE_NAME_PAIR}" "${Escape}[m")
endforeach()
message(STATUS "Hera device: Listing registered data types")
foreach(DATA_TYPE_NAME_PAIR ${DATA_TYPE_NAME_PAIRS})
  message(STATUS "\t- ${Escape}[34m" "${DATA_TYPE_NAME_PAIR}" "${Escape}[m")
endforeach()

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
