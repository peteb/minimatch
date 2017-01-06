#include <iostream>
#include <glog/logging.h>

#include "framework/services.h"
#include "matcher/matcher.h"
#include "messaging/loopback_service.h"

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);

  loopback_messaging_init();
  matcher_init();
  auto matcher = (matching_engine_t *)find_service("matcher");
}
