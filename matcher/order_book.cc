#include "order_book.h"

#include <glog/logging.h>
#include <algorithm>

order_book::order_book(const char *ins_id) {
  LOG(INFO) << "Creating order book '" << ins_id << "'";
  latest_order_id = 0;
}

order_book::~order_book() {
  LOG(INFO) << "Shutting down order book '" << ins_id << "'";
  // TODO: cancel outstanding orders
}

status_t order_book::limit_order(int side, unsigned quantity, uint64_t price, uint64_t *order_id) {
  order new_order;
  new_order.side = side;
  new_order.quantity = quantity;
  new_order.price = price;
  new_order.order_id = 0;

  order remaining = fill_order(new_order);
  if (remaining.quantity == 0) {
    *order_id = new_order.order_id;
    return SUC_EXECUTED;
  }

  new_order.order_id = allocate_order_id();
  *order_id = new_order.order_id;
  save_order(new_order);
  return SUC_INBOOK;
}

status_t order_book::register_callback(void *opaque, matching_engine_callback_t *callback) {
  callbacks.insert({opaque, callback});
  return SUC_OK;
}

order order_book::fill_order(const order &original_order) {
  order remaining = original_order;

  // TODO: this will probably need to be sped-up using a heap or something.

  std::vector<order> &order_collection = (original_order.side == SIDE_BUY ? sell_orders : buy_orders);
  auto iter = begin(order_collection);
  while (iter != order_collection.end()) {
    if (!iter->can_fill(original_order)) {
      ++iter;
      continue;
    }

    unsigned quantity = std::min(iter->quantity, remaining.quantity);
    iter->quantity -= quantity;
    remaining.quantity -= quantity;

    // Send report for the order book order
    order_match_report_t first_report;
    first_report.ins_id = ins_id.c_str();
    first_report.order_id = iter->order_id;
    first_report.quantity = quantity;
    first_report.price = iter->price;

    for (auto p : callbacks) {
      p.second->order_matched(p.first, &first_report);
    }

    // ... and for the new order
    order_match_report_t second_report;
    second_report.ins_id = ins_id.c_str();
    second_report.order_id = remaining.order_id;
    second_report.quantity = quantity;
    second_report.price = iter->price;

    for (auto p : callbacks) {
      p.second->order_matched(p.first, &second_report);
    }

    if (iter->quantity == 0) {
      iter = order_collection.erase(iter);
    }

    if (remaining.quantity == 0) {
      break;
    }
  }

  return remaining;
}

void order_book::save_order(const order &order) {
  if (order.side == SIDE_BUY) {
    buy_orders.push_back(order);
  }
  else {
    sell_orders.push_back(order);
  }
}

uint64_t order_book::allocate_order_id() {
  ++latest_order_id;
}
