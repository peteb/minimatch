// -*- c++ -*-

#ifndef _MATCHER_ORDER_BOOK_H
#define _MATCHER_ORDER_BOOK_H

#include <string>
#include <vector>
#include <set>

#include "framework/services.h"

class order {
public:
  bool can_fill(const order &other) const {
    if (quantity == 0) {
      return false;
    }

    if (other.side == SIDE_SELL && side == SIDE_BUY) {
      return price >= other.price;
    }
    else if (other.side == SIDE_BUY && side == SIDE_SELL) {
      return price <= other.price;
    }

    return false;
  }

  int side;
  unsigned quantity;
  uint64_t price;
  uint64_t order_id;
};

class order_book {
public:
  order_book(const char *ins_id);
  ~order_book();

  status_t limit_order(int side, unsigned quantity, uint64_t price, uint64_t *order_id);
  status_t register_callback(void *opaque, matching_engine_callback_t *callback);

private:
  order fill_order(const order &original_order);
  void save_order(const order &order);
  uint64_t allocate_order_id();

private:
  std::string ins_id;
  uint64_t latest_order_id;

  std::vector<order> sell_orders;
  std::vector<order> buy_orders;
  std::set<std::pair<void *, matching_engine_callback_t *>> callbacks;
};

#endif // !_MATCHER_ORDER_BOOK_H
