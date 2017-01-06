#include <glog/logging.h>

#include "api/apidef_generated.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <cstring>
#include <cstdlib>
#include <iostream>
#include <iomanip>

static int64_t total_bytes_written = 0;
static int64_t total_messages_written = 0;

static void timeoutcb(evutil_socket_t fd, short what, void *arg) {
  struct event_base *base = reinterpret_cast<struct event_base *>(arg);
  LOG(INFO) << "Timeout -- done";

  event_base_loopexit(base, NULL);
}

static void writecb(struct bufferevent *bev, void *ctx) {
  static flatbuffers::FlatBufferBuilder builder(200);
  static int order_id = 0;
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

  unsigned short data_length = builder.GetSize();
  evbuffer_add(bufferevent_get_output(bev), &data_length, 2);
  evbuffer_add(bufferevent_get_output(bev), builder.GetBufferPointer(), builder.GetSize());

  ++total_messages_written;
  total_bytes_written += evbuffer_get_length(bufferevent_get_output(bev));
}

static void eventcb(struct bufferevent *bev, short events, void *ptr) {
  if (events & BEV_EVENT_CONNECTED) {
    int one = 1;
    setsockopt(bufferevent_getfd(bev), IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    LOG(INFO) << "Connected";

    // Kick-start the connection with some data...
    writecb(bev, nullptr);
  }
  else if (events & BEV_EVENT_ERROR) {
    LOG(ERROR) << "Received error";
  }
}

int main(int argc, const char **argv) {
  google::InitGoogleLogging(argv[0]);
  FLAGS_logtostderr = 1;

  if (argc != 4) {
    std::cerr << "Usage: client <address> <port> <time>" << std::endl;
    return EXIT_FAILURE;
  }

  const char *host = argv[1];
  int port = atoi(argv[2]);
  int seconds = atoi(argv[3]);

  struct event_base *base = event_base_new();
  if (!base) {
    LOG(ERROR) << "Failed to open event base";
    return EXIT_FAILURE;
  }

  // Setup timeout event
  struct timeval timeout;
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;

  struct event *evtimeout = evtimer_new(base, timeoutcb, base);
  evtimer_add(evtimeout, &timeout);

  // Connect
  struct sockaddr_in sin;
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  if (!inet_aton(host, &sin.sin_addr)) {
    LOG(ERROR) << "Failed to parse host address";
    return EXIT_FAILURE;
  }

  struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bev, nullptr, writecb, eventcb, NULL);
  bufferevent_enable(bev, EV_READ|EV_WRITE);

  if (bufferevent_socket_connect(bev, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
    bufferevent_free(bev);
    LOG(ERROR) << "Failed to connect";
    return EXIT_FAILURE;
  }

  // Cleanup and report
  event_base_dispatch(base);

  bufferevent_free(bev);
  event_free(evtimeout);
  event_base_free(base);

  std::cout << std::setw(30) << "Total bytes written: " << total_bytes_written << std::endl;
  std::cout << std::setw(30) << "Total messages written: " << total_messages_written << std::endl;

  return EXIT_SUCCESS;
}
