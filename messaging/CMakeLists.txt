# -*- cmake -*-

add_library(messaging messaging_service.cc loopback_service.cc multicast_connection.cc)
target_link_libraries(messaging ${GLOG_LIBRARIES} framework)
