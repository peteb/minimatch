# -*- cmake -*-

add_library(matcher matcher.cc order_book.cc)
target_link_libraries(matcher ${GLOG_LIBRARIES} framework)
