#include "matcher.h"
#include "order_book.h"
#include "framework/services.h"
#include "utils/memory.h"
#include "api/apidef_generated.h"

#include <glog/logging.h>
#include <map>
#include <string>
#include <memory>

static status_t dec_in_price(const char *ins_id, int *dec);
static status_t limit_order(const char *ins_id, int side, unsigned quantity, uint64_t price, uint64_t *order_id);
static status_t register_callback(const char *ins_id, void *opaque, matching_engine_callback_t *callback);

static status_t received_message(const void *data, size_t size);

typedef std::map<std::string, std::unique_ptr<order_book>> order_book_map;
static matching_engine_t service;
static order_book_map order_books;
static messaging_callback_t messaging_cb;
static messaging_t *messaging;

static order_book *fetch_order_book(const char *ins_id) {
  auto iter = order_books.find({ins_id});
  if (iter != order_books.end()) {
    return iter->second.get();
  }
  else {
    order_book_map::value_type pair = {{ins_id}, make_unique<order_book>(ins_id)};
    return order_books.insert(std::move(pair)).first->second.get();
  }
}

void matcher_init() {
  LOG(INFO) << "Initializing matcher";

  service.dec_in_price = dec_in_price;
  service.limit_order = limit_order;
  service.register_callback = register_callback;

  register_service("matcher", &service);

  messaging = reinterpret_cast<messaging_t *>(find_service("messaging"));
  messaging_cb.received_message = received_message;
  messaging->register_callback(&messaging_cb, 0);
}

void matcher_shutdown() {
  LOG(INFO) << "Shutting down matcher";
  order_books.clear();
  unregister_service("matcher", &service);
}

// --
status_t dec_in_price(const char *ins_id, int *dec) {
  *dec = 2;
  return SUC_OK;
}

// --
status_t limit_order(const char *ins_id, int side, unsigned quantity, uint64_t price, uint64_t *order_id) {
  return fetch_order_book(ins_id)->limit_order(side, quantity, price, order_id);
}

// --
status_t register_callback(const char *ins_id, void *opaque, matching_engine_callback_t *callback) {
  return fetch_order_book(ins_id)->register_callback(opaque, callback);
}

// --
status_t received_message(const void *data, size_t size) {
  const api::Message *msg = api::GetMessage(data);
  if (msg->Body_type() == api::MessageType_LimitOrder) {
    auto order = static_cast<const api::LimitOrder *>(msg->Body());

    if (order->local_id() % 10000) {
      return SUC_OK;
    }

    LOG(INFO) << "limit order side=" << order->side()
              << ", quantity=" << order->quantity()
              << ", price=" << order->price()
              << ", local_id=" << order->local_id();
  }

  return SUC_OK;
}
