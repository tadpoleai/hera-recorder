if(NOT HERA_INSTALL_PREFIX)
  message(STATUS "FindHera.cmake: Setting HERA_INSTALL_PREFIX to '/usr/local/'")
  set(HERA_INSTALL_PREFIX /usr/local/)
endif()

set(HERA_INCLUDE_DIRS ${HERA_INSTALL_PREFIX}/include)
set(HERA_LIBRARY_DIR ${HERA_INSTALL_PREFIX}/lib)

link_directories(${HERA_LIBRARY_DIR})

set(HERA_LIBS
    ${HERA_LIBRARY_DIR}/libhera-common.so
    ${HERA_LIBRARY_DIR}/libhera-storage.so
    ${HERA_LIBRARY_DIR}/libhera-device.so)

set(HERA_DRIVER_LIBS
    ${HERA_LIBRARY_DIR}/libhera-common.so
    ${HERA_LIBRARY_DIR}/libhera-storage.so
    ${HERA_LIBRARY_DIR}/libhera-device-driver.so)

set(HERA_DRIVER_ARM_LIBS
    ${HERA_LIBRARY_DIR}/libhera-common.so
    ${HERA_LIBRARY_DIR}/libhera-storage.so
    ${HERA_LIBRARY_DIR}/libhera-device-driver.so
    ${HERA_LIBRARY_DIR}/libvsdk_ar0143.so)
