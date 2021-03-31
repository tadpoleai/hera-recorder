set(HERA_INCLUDE_DIRS ${HERA_INSTALL_PREFIX}/include)
set(HERA_LIBRARY_DIR ${HERA_INSTALL_PREFIX}/lib)

link_directories(${HERA_LIBRARY_DIR})

set(HERA_LIBS
    ${HERA_LIBRARY_DIR}/libhera-common.so
    ${HERA_LIBRARY_DIR}/libhera-storage.so
    ${HERA_LIBRARY_DIR}/libhera-device.so
    ${HERA_LIBRARY_DIR}/libhera-convert-ros-message.so)
