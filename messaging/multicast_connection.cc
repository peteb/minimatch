#include "multicast_connection.h"
#include "utils/variables.h"
#include "utils/timing.h"

#include <glog/logging.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <chrono>
#include <ratio>

#include "api/apidef_generated.h"

#define MAX_BUFFER_SIZE 10000

// --
static void readcb(evutil_socket_t sock, short events, void *opaque);
static void writecb(evutil_socket_t sock, short events, void *opaque);

// --
typedef struct multicast_header {
  uint64_t seq_num;
  uint16_t seq_id;
  uint16_t size;
} multicast_header_t;

// --
multicast_connection::multicast_connection(const char *group_address, uint16_t port) {
  remote_addr.sin_family = AF_INET;
  remote_addr.sin_port = htons(port);

  if (!inet_aton(group_address, &remote_addr.sin_addr)) {
    LOG(ERROR) << "Failed to parse address " << group_address;
    std::abort();
  }

  local_addr.sin_family = AF_INET;
  local_addr.sin_addr.s_addr = 0;
  local_addr.sin_port = htons(port);

  sync_send = read_variable<bool>("MSG_SYNC_SEND", true);
}

// --
void multicast_connection::join(struct event_base *read_base, struct event_base *write_base) {
  if (read_event) {
    LOG(ERROR) << "Connection already joined";
    std::abort();
  }

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  evutil_make_socket_nonblocking(sock);

  int reuseaddr = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0) {
    LOG(ERROR) << "Failed to set SO_REUSEADDR: " << strerror(errno);
    std::abort();
  }

  if (bind(sock, reinterpret_cast<struct sockaddr *>(&local_addr), sizeof(local_addr)) < 0) {
    LOG(ERROR) << "Failed to bind: " << strerror(errno);
    std::abort();
  }

  if (read_base) {
    int loop = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) < 0) {
      LOG(ERROR) << "Failed to set multicast loop: " << strerror(errno);
      std::abort();
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr = remote_addr.sin_addr;
    mreq.imr_interface = local_addr.sin_addr;

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) << 0) {
      LOG(ERROR) << "Failed to subscribe to multicast group: " << strerror(errno);
      std::abort();
    }

    // Join the event loop
    read_event = event_new(read_base, sock, EV_READ|EV_PERSIST, ::readcb, this);
    if (!read_event) {
      LOG(ERROR) << "Failed to create read event";
      std::abort();
    }

    event_add(read_event, nullptr);
  }

  if (write_base) {
    write_event = event_new(write_base, sock, EV_WRITE|EV_PERSIST, ::writecb, this);
    if (!write_event) {
      LOG(ERROR) << "Failed to create write event";
      std::abort();
    }

    event_add(write_event, nullptr);
  }
}

// --
void multicast_connection::leave() {
  if (!sock) {
    LOG(WARNING) << "Connection not joined";
    return;
  }

  evutil_closesocket(sock);

  if (read_event) {
    event_free(read_event);
    read_event = nullptr;
  }

  if (write_event) {
    event_free(write_event);
    write_event = nullptr;
  }
}

// --
void multicast_connection::readcb(evutil_socket_t sock, short events) {
  char buffer[MAX_BUFFER_SIZE];
  struct sockaddr_in src_addr;
  socklen_t len;

  int remaining_bytes = recvfrom(sock, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr *>(&src_addr), &len);

  if (remaining_bytes <= sizeof(multicast_header_t)) {
    LOG(ERROR) << "Failure during read: not enough bytes";
    return;
  }

  char *cursor = buffer;

  while (remaining_bytes > 0) {
    // NOTE: expecting host endian for header fields
    multicast_header_t *hdr = reinterpret_cast<multicast_header_t *>(cursor);
    uint64_t seq_num = hdr->seq_num;
    uint16_t seq_id = hdr->seq_id;

    if (seq_id == sequences.local_id()) {
      if (seq_num == in_flight_seq_num) {
        in_flight_seq_num = 0;
      }

      last_rx_seq_num = seq_num;
    }

    remaining_bytes -= sizeof(multicast_header_t) + hdr->size;
    int seq_diff = sequences.check(seq_id, seq_num);

    if (seq_diff <= 0) {
      LOG(WARNING) << "Detected duplicate seq_id=" << seq_id << ", seq_num=" << seq_num;
      continue;
    }
    else if (seq_diff > 1) {
      LOG(WARNING) << "Detected gap, seq_id=" << seq_id << ", seq_num=" << seq_num << ", seq_diff=" << seq_diff;
      continue;
    }

    void *data = cursor + sizeof(multicast_header_t);
    size_t size = hdr->size;

    if (on_read) {
      on_read(data, size);
    }

    cursor += sizeof(multicast_header_t) + hdr->size;
    sequences.commit(seq_id, seq_num);
  }
}

// --
void multicast_connection::writecb(evutil_socket_t sock, short events) {
  std::lock_guard<std::mutex> lock(tx_queue_m);
  if (tx_queue.empty() || (sync_send && in_flight_seq_num != 0)) {
    return;
  }

  static char mtu[MAX_BUFFER_SIZE];
  int space_remaining = MAX_BUFFER_SIZE;
  char *cursor = mtu;
  uint16_t seq_id = sequences.local_id();
  uint64_t last_seq_num = 0;

  while (!tx_queue.empty()) {
    auto item = tx_queue.front();
    size_t total_size = item.size() + sizeof(multicast_header_t);

    if (space_remaining < total_size) {
      break;
    }

    last_seq_num = sequences.alloc();

    multicast_header_t *header = reinterpret_cast<multicast_header_t *>(cursor);
    header->seq_num = last_seq_num;
    header->seq_id = seq_id;
    header->size = item.size();

    cursor += sizeof(multicast_header_t);
    memcpy(cursor, item.data(), item.size());
    cursor += item.size();

    tx_queue.pop();
    space_remaining -= total_size;
  }

  transmit_message(mtu, cursor - mtu);
  sequences.commit(last_seq_num);
}

// --
uint64_t multicast_connection::send_now(const void *data, size_t size, int flags) {
  if (size > MAX_BUFFER_SIZE - sizeof(multicast_header_t)) {
    LOG(ERROR) << "Buffer overrun";
    std::abort();
  }

  char buffer[MAX_BUFFER_SIZE];
  multicast_header_t *hdr = reinterpret_cast<multicast_header_t *>(buffer);
  uint64_t new_seq = sequences.alloc();
  hdr->seq_num = new_seq;
  hdr->seq_id = sequences.local_id();
  hdr->size = size;

  memcpy(buffer + sizeof(multicast_header_t), data, size);

  transmit_message(buffer, size + sizeof(multicast_header_t));

  if (flags & SEND_SYNC) {
    while (last_rx_seq_num != new_seq);
  }

  sequences.commit(new_seq);
  last_tx_seq_num = new_seq;

  return new_seq;
}

// --
void multicast_connection::transmit_message(const void *data, size_t size) {
  while (true) {
    int n = sendto(sock,
                   data,
                   size,
                   0,
                   reinterpret_cast<struct sockaddr *>(&remote_addr),
                   sizeof(remote_addr));
    if (n < 0) {
      if (errno == EAGAIN) {
        continue;
      }

      LOG(ERROR) << "Failed to send multicast message: " << strerror(errno);
      std::abort();
    }
    break;
  }
}


// --
void multicast_connection::send(const void *data, size_t size) {
  if (write_event) {
    const char *ptr = reinterpret_cast<const char *>(data);
    std::vector<char> v(ptr, ptr + size);

    std::lock_guard<std::mutex> lock(tx_queue_m);
    tx_queue.emplace(v);
  }
  else {
    send_now(data, size, sync_send ? SEND_SYNC : 0);
  }
}

// --
void readcb(evutil_socket_t sock, short events, void *opaque) {
  static timing<5000, 20, uint16_t> statistics(__func__);

  statistics.benchmark<void>([=]{
    static_cast<multicast_connection *>(opaque)->readcb(sock, events);
  });
}

// --
void writecb(evutil_socket_t sock, short events, void *opaque) {
  static timing<5000, 20, uint16_t> statistics(__func__);

  statistics.benchmark<void>([=]{
    static_cast<multicast_connection *>(opaque)->writecb(sock, events);
  });
}
