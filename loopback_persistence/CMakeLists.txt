# -*- cmake -*-

add_library(loopback_persistence init.cc)
target_link_libraries(loopback_persistence ${GLOG_LIBRARIES} framework)
