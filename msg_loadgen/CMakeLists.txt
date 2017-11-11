# -*- cmake -*-

add_executable(msg_loadgen main.cc)

find_package(LibEvent REQUIRED)
find_package(GLOG REQUIRED)

target_link_libraries(msg_loadgen ${GLOG_LIBRARIES} ${LIBEVENT_LIB} messaging)

add_dependencies(msg_loadgen api)
