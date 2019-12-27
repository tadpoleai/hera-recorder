# Find git
string(ASCII 27 Escape)
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

  set(GIT_INFO_CPPSRC "git_info.cpp")
  add_custom_command(
    OUTPUT ${GIT_INFO_CPPSRC}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/.git/logs/HEAD"
            # COMMAND echo "`" "git" "rev-parse" "HEAD" "|" "sed" "s/^/Git\
            # HEAD\ is\ now:\ /" "`" COMMAND "git" "rev-parse" "--sq" "HEAD" "|"
            # "sed" "s/^/const\ char\ *GIT_COMMIT_HEAD\ =\ \\\"/" "|" "sed"
            # "s/\\'\ /\\\"\;/"
            # "|" "sed" "s/\\'//" ">" "${GIT_INFO_CPPSRC}")
    COMMAND
      echo "`" "git" "log" "--decorate=short" "-1" "|" "head" "-1" "|" "sed"
      "s/^/Git\ HEAD\ is\ now:\ /" "`"
    COMMAND
      "git" "log" "--decorate=short" "-1" "|" "head" "-1" "|" "sed"
      "s/^/const\ char\ *GIT_COMMIT_HEAD\ =\ \\\"/" "|" "sed" "s/\\)/\\)\\\"\;/" ">" "${GIT_INFO_CPPSRC}")

  include(deploy/cmake/cpp_common.cmake)
  add_library(hera-common-git-info STATIC ${GIT_INFO_CPPSRC})
endif()
