# Find git
string(ASCII 27 Escape)

set(GIT_INFO_CPPSRC "${CMAKE_CURRENT_BINARY_DIR}/git_info.cpp")
set(GIT_INFO_TS "${CMAKE_SOURCE_DIR}/client/git_info.ts")

string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
message(STATUS "BUILD TIME: " ${BUILD_TIMESTAMP})

if(force-git-info)
  message(STATUS "${Escape}[32m" "Git version set to ${force-git-info}"
                 "${Escape}[m")
  set(GIT_INFO_ENABLED 1)
  add_definitions(-DGIT_INFO_ENABLED)

  file(
    WRITE ${GIT_INFO_CPPSRC}
    "const char *GIT_COMMIT_HEAD = \"${force-git-info} - built ${BUILD_TIMESTAMP}\";"
  )
  file(WRITE ${GIT_INFO_TS}
       "export const GitVersion = \"${force-git-info} - built ${BUILD_TIMESTAMP}\";")

  include(deploy/cmake/cpp_common.cmake)
  add_library(hera-git-info STATIC ${GIT_INFO_CPPSRC})

else()
  execute_process(
    COMMAND "which" "git"
    OUTPUT_VARIABLE GIT_PATH
    RESULT_VARIABLE GIT_CHECK)

  if(GIT_PATH STREQUAL "")
    message(
      STATUS "${Escape}[31m"
             "Git not found, the repository is not obtained from git, ignoring"
             "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m" "Git found" "${Escape}[m")
    set(GIT_INFO_ENABLED 1)
    add_definitions(-DGIT_INFO_ENABLED)

    add_custom_command(
      OUTPUT ${GIT_INFO_CPPSRC}
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/.git/logs/HEAD"
      COMMAND
        echo "`" "git" "log" "--decorate=short" "-1" "|" "head" "-1" "|" "sed"
        "s/^/Git\ HEAD\ is\ now:\ /" "`"
      COMMAND
        "git" "log" "--decorate=short" "-1" "|" "head" "-1" "|" "sed"
        "s/^/const\ char\ *GIT_COMMIT_HEAD\ =\ \\\"/" "|" "sed"
        "s/\\)/\\) - built ${BUILD_TIMESTAMP}\\\"\;/" ">" "${GIT_INFO_CPPSRC}"
      COMMAND
        "git" "log" "--decorate=short" "-1" "|" "head" "-1" "|" "sed"
        "s/^/export\ const\ GitVersion\ =\ \\\"/" "|" "sed"
        "s/\\)/\\) - built ${BUILD_TIMESTAMP}\\\"\;/" ">" "${GIT_INFO_TS}")

    include(deploy/cmake/cpp_common.cmake)
    add_library(hera-git-info STATIC ${GIT_INFO_CPPSRC})
  endif()
endif()
