string(ASCII 27 Escape)
message(STATUS "${Escape}[32m" "Linking S32V Extra Depends" "${Escape}[m")
message(STATUS "${Escape}[32m" "Use \${S32V_EXTRA_LIBS}" "${Escape}[m")

link_directories(
  /opt/s32v/extra-depends/3rd/import/lib
  /opt/s32v/extra-depends/3rd/import/usr/local/lib
  /opt/s32v/extra-depends/3rd/ros_aarch64/lib
  /opt/s32v/extra-depends/3rd/nmealib/lib
  /opt/s32v/extra-depends)
set(NEBULA_COMMON_3RD_LIBS
    yaml-cpp
    log4cplus
    leveldb
    proj
    gflags
    glog
    protobuf
    boost_system
    boost_filesystem
    boost_system-mt
    boost_regex
    boost_thread-mt
    boost_atomic-mt
    boost_signals-mt
    boost_filesystem-mt
    boost_regex-mt
    boost_chrono-mt
    boost_date_time-mt)
set(NEBULA_ROS_LIBS
    rosconsole_glog
    rosconsole_print
    rosconsole_backend_interface
    xmlrpcpp
    cpp_common
    console_bridge
    actionlib
    message_filters
    rostime
    roscpp
    roscpp_serialization
    rosconsole
    tf2
    tf2_ros)
set(NEBULA_PROTO_LIBS gnssProto ubloxGpsProto)
set(NEBULA_LIBS S32Sal ublox_gps ublox_msgs)

set(S32V_EXTRA_LIBS ${NEBULA_COMMON_3RD_LIBS} ${NEBULA_ROS_LIBS}
                    ${NEBULA_PROTO_LIBS} ${NEBULA_LIBS})
