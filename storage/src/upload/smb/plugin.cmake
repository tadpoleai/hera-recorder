# Find LibNfs
string(ASCII 27 Escape)

if(ARCHITECTURE STREQUAL "x86_64")
  find_package(PkgConfig)
  pkg_search_module(smbclient smbclient)

  if(${smbclient_FOUND})
    set(PLUGIN_DEP_FOUND 1)
    set(PLUGIN_DEP_INCLUDE ${smbclient_INCLUDE_DIRS})
    set(PLUGIN_DEP_LIBS ${smbclient_LIBRARIES})

    message_plugin("smbclient found")
    message_plugin(${smbclient_INCLUDE_DIRS})
    message_plugin(${smbclient_LIBRARIES})
  else()
  set(PLUGIN_DEP_FOUND 0)
    message_plugin("smbclient not found")
  endif()
else()
  message_plugin("smbclient not buildable for ${ARCHITECTURE}")
endif()
