# Find LibNfs
string(ASCII 27 Escape)

if(ARCHITECTURE STREQUAL "x86_64")
  set(LIBNFS_INCLUDE_DIR /usr/local/include/nfsc/)
  set(LIBNFS_LIBRARIES /usr/local/lib/libnfs.a)

  if(EXISTS ${LIBNFS_LIBRARIES} AND EXISTS ${LIBNFS_INCLUDE_DIR}/libnfs.h)
    set(PLUGIN_DEP_FOUND 1)
    set(PLUGIN_DEP_INCLUDE ${LIBNFS_INCLUDE_DIR})
    set(PLUGIN_DEP_LIBS ${LIBNFS_LIBRARIES})

    message_plugin("LibNfs found")
    message_plugin(${LIBNFS_INCLUDE_DIR})
    message_plugin(${LIBNFS_LIBRARIES})
  else()
    set(PLUGIN_DEP_FOUND 0)
    message_plugin("LibNfs not found")
  endif()
else()
  message_plugin("LibNFS not buildable for ${ARCHITECTURE}")
endif()
