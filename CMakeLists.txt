# -* cmake -*-
cmake_minimum_required(VERSION 2.6)
project(minimatch)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake_modules)

add_definitions(-std=c++11)
enable_testing()

find_package(GLOG REQUIRED)
find_package(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS}) # TODO: pull out test suite into own project
include_directories(.)

add_subdirectory(framework)
add_subdirectory(matcher)
add_subdirectory(fbgw)
add_subdirectory(api)
add_subdirectory(fb_loadgen)
add_subdirectory(messaging)

set(MODULES ${GLOG_LIBRARIES} framework matcher messaging)

add_executable(minimatch main.cc)
target_link_libraries(minimatch ${MODULES})

# Tests for matching engine implementation
add_executable(matching_engine_test matching_engine_test.cc)
target_link_libraries(matching_engine_test ${GTEST_BOTH_LIBRARIES} pthread ${MODULES})
add_test(MatchingEngine matching_engine_test)
