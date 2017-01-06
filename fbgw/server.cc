#include "server.h"
#include "api/apidef_generated.h"
#include "framework/services.h"

#include <glog/logging.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>

static void do_read(evutil_socket_t fd, short events, void *arg);
static void do_write(evutil_socket_t fd, short events, void *arg);

static messaging_t *messaging;

static void readcb(struct bufferevent *bev, void *ctx) {
  struct evbuffer *input, *output;
  char *line;
  size_t n;
  int i;
  input = bufferevent_get_input(bev);
  output = bufferevent_get_output(bev);


  while (1) {
    evbuffer_iovec vecs[1];
    if (!evbuffer_peek(input, sizeof(unsigned short), nullptr, vecs, 1)) {
      return;
    }

    if (vecs[0].iov_len < sizeof(unsigned short)) {
      return;
    }

    unsigned short data_length = *reinterpret_cast<unsigned short *>(vecs[0].iov_base);

    if (evbuffer_get_length(input) >= data_length + sizeof(unsigned short)) {
      // NOTE: a copy is being made here, either skip this copy or the one in messaging
      char buf[1024];
      int n = evbuffer_remove(input, buf, data_length + sizeof(unsigned short));
      messaging->send_message(buf + 2, n - 2);
      // TODO: verify data

    }
    else {
      return;
    }
  }
}

static void errorcb(struct bufferevent *bev, short error, void *ctx) {
  if (error & BEV_EVENT_EOF) {

  }
  else if (error & BEV_EVENT_ERROR) {

  }
  else if (error & BEV_EVENT_TIMEOUT) {

  }

  bufferevent_free(bev);
}

void do_accept(evutil_socket_t listener, short event, void *arg) {
  struct event_base *base = static_cast<struct event_base *>(arg);
  struct sockaddr_storage ss;
  socklen_t slen = sizeof(ss);
  int fd = accept(listener, reinterpret_cast<struct sockaddr *>(&ss), &slen);

  if (fd < 0) {
    LOG(ERROR) << "accept: " << strerror(errno);
  }
  else if (fd > FD_SETSIZE) {
    close(fd);
  }
  else {
    LOG(INFO) << "Connection from client established";
    struct bufferevent *bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);
    bufferevent_enable(bev, EV_READ|EV_WRITE);

    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
  }
}

void fbgw_run() {
  messaging = reinterpret_cast<messaging_t *>(find_service("messaging"));

  evutil_socket_t listener;
  struct sockaddr_in sin;
  struct event_base *base;

  base = event_base_new();
  if (!base) {
    LOG(ERROR) << "Failed to setup event_base";
    return;
  }

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(48400);

  listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

  {
    int reuseaddr = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
  }

  if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    LOG(ERROR) << "bind: " << strerror(errno);
    return;
  }

  if (listen(listener, 16) < 0) {
    LOG(ERROR) << "listen: " << strerror(errno);
    return;
  }

  struct event *listener_event = event_new(base, listener, EV_READ|EV_PERSIST, do_accept, (void*)base);
  event_add(listener_event, NULL);

  LOG(INFO) << "Started server at port " << ntohs(sin.sin_port);
  event_base_dispatch(base);
}
