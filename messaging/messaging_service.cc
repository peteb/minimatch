#include "messaging_service.h"
#include "multicast_connection.h"

#include "framework/services.h"
#include "utils/memory.h"
#include "utils/thread.h"
#include "utils/variables.h"
#include "utils/timing.h"

#include <glog/logging.h>
#include <mutex>
#include <set>
#include <thread>
#include <cassert>

#include <event2/event.h>
#include <iostream>

// --
static status_t register_callback(messaging_callback_t *cb, void *opaque);
static status_t send_message(const void *data, size_t size);
static void     on_read(const void *data, size_t size);

// --
static messaging_t service = {
  register_callback,
  send_message
};

// --
static std::unique_ptr<multicast_connection> mc_conn;
static std::thread eventloop;

static std::mutex cb_mutex;
static std::set<messaging_callback_t *> callbacks;

static struct event_base *base = nullptr;

// --
void messaging_init() {
  LOG(INFO) << "Initializing messaging";
  register_service("messaging", &service);

  base = event_base_new();
  assert(base && "Failed to create event base");

  mc_conn = make_unique<multicast_connection>("239.0.0.1", 40100);
  mc_conn->on_read = on_read;

  if (read_variable<bool>("MSG_BUFFER_TX", true)) {
    mc_conn->join(base, base);
  }
  else {
    mc_conn->join(base, nullptr);
  }

  eventloop = std::move(cpu_thread("messaging", event_base_dispatch, base));
}

// --
void messaging_shutdown() {
  LOG(INFO) << "Shutting down messaging";
  unregister_service("messaging", &service);

  event_base_loopexit(base, nullptr);
  eventloop.join();

  if (mc_conn) {
    mc_conn->leave();
    mc_conn.reset();
  }
}

// --
status_t register_callback(messaging_callback_t *cb, void *opaque) {
  std::lock_guard<std::mutex> lock(cb_mutex);
  callbacks.insert(cb);
  return SUC_OK;
}

// --
status_t send_message(const void *data, size_t size) {
  assert(mc_conn && "No multicast connection established");
  mc_conn->send(data, size);
  return SUC_OK;
}

// --
void on_read(const void *data, size_t size) {
  static stream_measure measure("msg_rx");
  measure.collect(size);

  std::lock_guard<std::mutex> lock(cb_mutex);

  for (auto &cb : callbacks) {
    cb->received_message(data, size);
  }
}
