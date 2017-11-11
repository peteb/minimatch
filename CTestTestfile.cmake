# CMake generated Testfile for 
# Source directory: /root/code/minimatch
# Build directory: /root/code/minimatch
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
ADD_TEST(MatchingEngine "matching_engine_test")
SUBDIRS(framework)
SUBDIRS(matcher)
SUBDIRS(fbgw)
SUBDIRS(api)
SUBDIRS(fb_loadgen)
SUBDIRS(msg_loadgen)
SUBDIRS(messaging)
