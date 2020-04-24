string(ASCII 27 Escape)
message(STATUS "${Escape}[32m" "Linking S32VSAL Depends" "${Escape}[m")
message(STATUS "${Escape}[32m" "Use \${S32VSAL_LIBS}" "${Escape}[m")

set(S32VSAL_INSTALL_DIR "/opt/s32v/s32vsal")
set(S32VSAL_INCLUDE_DIR ${S32VSAL_INSTALL_DIR}/include)
include_directories(${S32VSAL_INCLUDE_DIR})

set(S32VSAL_LIB_DIR ${S32VSAL_INSTALL_DIR}/lib)
set(S32VSAL_IMPORT_LIB_DIR ${S32VSAL_LIB_DIR}/3rd/import/lib)
set(S32VSAL_ROSAARCH_LIB_DIR ${S32VSAL_LIB_DIR}/3rd/ros_aarch64/lib)

set(NEBULA_COMMON_3RD_LIBS
    ${S32VSAL_IMPORT_LIB_DIR}/libyaml-cpp.so
    ${S32VSAL_IMPORT_LIB_DIR}/liblog4cplus.so
    ${S32VSAL_IMPORT_LIB_DIR}/libleveldb.so
    ${S32VSAL_IMPORT_LIB_DIR}/libconsole_bridge.so
    ${S32VSAL_IMPORT_LIB_DIR}/libproj.so
    ${S32VSAL_IMPORT_LIB_DIR}/libgflags.so
    ${S32VSAL_IMPORT_LIB_DIR}/libglog.so
    ${S32VSAL_IMPORT_LIB_DIR}/libprotobuf.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_system.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_filesystem.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_system-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_regex.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_thread-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_atomic-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_signals-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_filesystem-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_regex-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_chrono-mt.so
    ${S32VSAL_IMPORT_LIB_DIR}/libboost_date_time-mt.so)
set(NEBULA_ROS_LIBS
    ${S32VSAL_ROSAARCH_LIB_DIR}/librosconsole_glog.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/librosconsole_print.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/librosconsole_backend_interface.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libxmlrpcpp.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libcpp_common.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libactionlib.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libmessage_filters.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/librostime.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libroscpp.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libroscpp_serialization.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/librosconsole.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libtf2.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libtf2_ros.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libublox_gps.so
    ${S32VSAL_ROSAARCH_LIB_DIR}/libublox_msgs.so)

set(NEBULA_LIBS
    ${S32VSAL_LIB_DIR}/libcommonUtils.a
    ${S32VSAL_LIB_DIR}/libS32Sal.so
    ${S32VSAL_LIB_DIR}/libgnss.a
    ${S32VSAL_LIB_DIR}/libgnssProto.a
    ${S32VSAL_LIB_DIR}/libubloxGps.so
    ${S32VSAL_LIB_DIR}/libubloxGpsProto.a
    ${S32VSAL_LIB_DIR}/libubloxParser.a)

set(S32VSAL_VSDK_LIBS ${S32VSAL_LIB_DIR}/libvsdk_ov10635.so)

set(S32VSAL_LIBS ${NEBULA_LIBS} ${NEBULA_ROS_LIBS} ${NEBULA_COMMON_3RD_LIBS}
                 ${S32VSAL_VSDK_LIBS})
