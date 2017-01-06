#include "messaging/loopback_service.h"
#include "framework/services.h"
#include "utils/blocking_queue.h"
#include "utils/memory.h"

#include <glog/logging.h>
#include <thread>
#include <memory>
#include <set>

// --
static status_t register_callback(messaging_callback_t *cb, void *opaque);
static status_t send_message(const void *data, size_t size);

// --
typedef std::vector<char> message;
static std::thread looper;
static void eventloop();
static blocking_queue<std::unique_ptr<message>> messages;
static std::set<messaging_callback_t *> callbacks;

static messaging_t service;

// --
void loopback_messaging_init() {
  LOG(INFO) << "Initializing loopback messaging";

  service.register_callback = register_callback;
  service.send_message = send_message;

  register_service("loopback_messaging", &service);

  looper = std::move(std::thread(eventloop));
}

// --
void loopback_messaging_shutdown() {
  LOG(INFO) << "Shutting down loopback messaging";
  unregister_service("loopback_messaging", &service);
}

// --
status_t register_callback(messaging_callback_t *cb, void *opaque) {
  callbacks.insert(cb);
  return SUC_OK;
}

// --
status_t send_message(const void *data, size_t size) {
  // NOTE: copy is made
  const char *ptr = reinterpret_cast<const char *>(data);
  messages.push(make_unique<message>(ptr, ptr + size));

  return SUC_OK;
}

// --
void eventloop() {
  LOG(INFO) << "Loopback messaging thread starting";

  while (1) {
    auto message = messages.pop();

    for (auto &cb : callbacks) {
      cb->received_message(message->data(), message->size());
    }
  }

  LOG(INFO) << "Loopback messaging thread exiting";
}
