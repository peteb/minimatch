# -* cmake -*-

add_executable(fbgw main.cc server.cc)

find_package(LibEvent REQUIRED)
find_package(GLOG REQUIRED)

target_link_libraries(fbgw ${GLOG_LIBRARIES} ${LIBEVENT_LIB} matcher messaging pthread)
add_dependencies(fbgw api)
