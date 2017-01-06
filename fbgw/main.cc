#include <iostream>
#include <cstdlib>
#include <glog/logging.h>
#include <unistd.h>

#include "framework/services.h"
#include "matcher/matcher.h"
#include "messaging/messaging_service.h"
#include "messaging/loopback_service.h"
#include "utils/thread.h"

#include "server.h"

int main(int argc, char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  LOG(INFO) << "Starting FlatBuffer gw...";

  messaging_init();
  matcher_init();

  if (argc > 1 && strcmp(argv[1], "standby") == 0) {
    while (1) {
      pause();
    }
  }
  else {
    set_thread_name("fbgw-tcp");
    pin_current_thread();
    fbgw_run();
  }

  // Gw runs in the main thread
  // Matcher runs in the persistence thread for now (maybe separate out later)
  // The gw does throttling, session handling, authentication/authorization and
  // forwards the message to the persistence layer.
  // All communication with the matcher happens through the messaging layer.

  return EXIT_SUCCESS;
}
