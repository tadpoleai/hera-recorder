# Find npm
string(ASCII 27 Escape)
execute_process(
  COMMAND "npm" "--version" "-s"
  OUTPUT_VARIABLE NPM_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE NPM_CHECK)
if(${NPM_CHECK})
  message(STATUS "${Escape}[33m"
                 "Npm not found, invoking deploy/install-npm.sh to install"
                 "${Escape}[m")
  execute_process(
    COMMAND "sudo" "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/install-npm.sh"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../deploy/")
  # Check again
  execute_process(
    COMMAND "npm" "--version" "-s"
    OUTPUT_VARIABLE NPM_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE NPM_CHECK)
  if(${NPM_CHECK})
    message(FATAL_ERROR "${Escape}[31m"
                        "Npm installation failed! CMake aborted!" "${Escape}[m")
  else()
    message(STATUS "${Escape}[32m" "Npm ${NPM_VERSION} installation succeed"
                   "${Escape}[m")
  endif()
else()
  message(STATUS "${Escape}[32m" "Npm ${NPM_VERSION} found" "${Escape}[m")
endif()
