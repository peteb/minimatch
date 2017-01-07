// -*- c++ -*-

#ifndef _MESSAGING_MULTICAST_CONNECTION_H
#define _MESSAGING_MULTICAST_CONNECTION_H

#include <cstdint>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include <netinet/in.h>
#include <event2/event.h>

#include "sequence.h"

#define SEND_SYNC 0x0001

/*
 * Encapsulates setup and handling of multicast tx/rx.
 * Detects out-of-order, duplicates, etc.
 */
class multicast_connection {
public:
  multicast_connection(const char *group_address, uint16_t port);

  // Setup socket, join the event loop, ...
  void join(struct event_base *read_base, struct event_base *write_base);
  void leave();

  // Thread safe
  uint64_t send_now(const void *data, size_t size, int flags);
  void send(const void *data, size_t size);

  std::function<void(const void *, size_t)> on_read;

  void readcb(evutil_socket_t sock, short events);
  void writecb(evutil_socket_t sock, short events);

private:
  void transmit_message(const void *data, size_t size);

  struct sockaddr_in remote_addr;
  struct sockaddr_in local_addr;

  struct event *read_event = nullptr;
  struct event *write_event = nullptr;
  evutil_socket_t sock;

  std::mutex tx_queue_m;
  std::queue<std::vector<char>> tx_queue;

  uint64_t in_flight_seq_num;

  bool sync_send;

  sequence_numbers sequences;

  // Used in non-buffered mode
  std::atomic<uint64_t> last_tx_seq_num;
  alignas(64) std::atomic<uint64_t> last_rx_seq_num;
};

#endif // !_MESSAGING_MULTICAST_CONNECTION_H
