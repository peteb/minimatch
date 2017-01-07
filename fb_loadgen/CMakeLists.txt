# -*- cmake -*-

add_executable(fb_loadgen main.cc)

find_package(LibEvent REQUIRED)
find_package(GLOG REQUIRED)

target_link_libraries(fb_loadgen ${GLOG_LIBRARIES} ${LIBEVENT_LIB})

add_dependencies(fb_loadgen api)
