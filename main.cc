#include <iostream>
#include <glog/logging.h>

#include "framework/services.h"
#include "matcher/matcher.h"

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  matcher_init();
  auto matcher = (matching_engine_t *)find_service("matcher");
}
