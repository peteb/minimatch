# -*- cmake -*-

find_package(LibEvent REQUIRED)
find_package(Hiredis REQUIRED)

add_library(messaging messaging_service.cc loopback_service.cc multicast_connection.cc sequence.cc)

include_directories(${HIREDIS_INCLUDE_DIR})

target_link_libraries(messaging ${GLOG_LIBRARIES} ${LIBEVENT_LIB} pthread framework ${HIREDIS_LIBRARIES})
