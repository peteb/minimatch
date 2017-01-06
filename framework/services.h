#ifndef _FRAMEWORK_SERVICES_H
#define _FRAMEWORK_SERVICES_H

#include <stdint.h>
#include <stdlib.h>

// Error codes
typedef int status_t;
#define SUCCESS(status) ((status) >= 0)
#define FAILED(status) ((status) < 0)

#define SUC_OK           0
#define SUC_INBOOK       10001 /* Order put in order book */
#define SUC_EXECUTED     10002 /* Order completely executed */
#define ERR_NOINS        -10001 /* No such instrument */

#define SIDE_BUY  1
#define SIDE_SELL 2

// Matching engine
typedef struct order_match_report {
  const char *ins_id;
  uint64_t order_id;
  unsigned quantity;
  uint64_t price;
} order_match_report_t;

typedef struct matching_engine_callback {
  status_t (*order_matched)(void *opaque, order_match_report_t *report);
} matching_engine_callback_t;

typedef struct matching_engine {
  status_t (*register_callback)(const char *ins_id, void *opaque, matching_engine_callback_t *callback);
  status_t (*limit_order)(const char *ins_id, int side, unsigned quantity, uint64_t price, uint64_t *order_id);
  status_t (*cancel_order)(const char *ins_id, uint64_t order_id);
  status_t (*dec_in_price)(const char *ins_id, int *dec);
} matching_engine_t;


// Messaging
typedef struct messaging_callback {
  status_t (*received_message)(const void *data, size_t size);
} messaging_callback_t;

typedef struct messaging {
  status_t (*register_callback)(messaging_callback_t *callback, void *opaque);
  status_t (*send_message)(const void *data, size_t size);
} messaging_t;


// Library entry points
extern "C" void *find_service(const char *name);
extern "C" void register_service(const char *name, void *descriptor);
extern "C" void unregister_service(const char *name, void *descriptor);

#endif // !_FRAMEWORK_SERVICES_H
