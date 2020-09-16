# Find LibNfs
string(ASCII 27 Escape)

if(ARCHITECTURE STREQUAL "x86_64")
  set(LIBNFS_INCLUDE_DIR /usr/local/include/nfsc/)
  set(LIBNFS_LIBRARIES /usr/local/lib/libnfs.a)

  if(EXISTS ${LIBNFS_LIBRARIES} AND EXISTS ${LIBNFS_INCLUDE_DIR}/libnfs.h)
    add_definitions(-DLIBNFS_FOUND)
    include_directories(${LIBNFS_INCLUDE_DIR})

    message(STATUS "${Escape}[32m" "LibNFS found" "${Escape}[m")
  else()
    message(FATAL_ERROR "${Escape}[31m" "LibNFS not found! CMake aborted!"
                        "${Escape}[m")
  endif()
else()
  message(STATUS "${Escape}[33m" "LibNFS not buildable for ${ARCHITECTURE}"
                 "${Escape}[m")
endif()
