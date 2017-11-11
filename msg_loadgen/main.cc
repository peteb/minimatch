#include <glog/logging.h>

#include "api/apidef_generated.h"
#include "framework/services.h"
#include "messaging/messaging_service.h"
#include "utils/timing.h"

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <iomanip>

static int64_t total_bytes_written = 0;
static int64_t total_messages_written = 0;

int main(int argc, const char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  messaging_init();

  messaging_t *messaging = reinterpret_cast<messaging_t *>(find_service("messaging"));

  stream_measure measure("msg_tx");
  flatbuffers::FlatBufferBuilder builder(200);
  int order_id = 0;

  while (true) {
    builder.Clear();

    auto ins = builder.CreateString("ERICB");

    api::LimitOrderBuilder order_builder(builder);
    order_builder.add_local_id(++order_id);
    order_builder.add_price(23000);
    order_builder.add_quantity(80);
    order_builder.add_ins_id(ins);
    order_builder.add_side(api::SideType_Buy);
    auto order = order_builder.Finish();

    auto root = api::CreateMessage(builder, api::MessageType_LimitOrder, order.Union());
    builder.Finish(root);

    messaging->send_message(builder.GetBufferPointer(), builder.GetSize());
    measure.collect(builder.GetSize());
  }

  return EXIT_SUCCESS;
}
