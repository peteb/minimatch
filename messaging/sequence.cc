#include "sequence.h"
#include "utils/variables.h"

#include <hiredis/hiredis.h>
#include <glog/logging.h>
#include <cassert>
#include <limits>

// --
sequence_numbers::sequence_numbers() {
  redisContext *ctx = redisConnect(read_variable<const char *>("MSG_ID_SOURCE_ADDR", "127.0.0.1"), read_variable("MSG_ID_SOURCE_PORT", 6379));

  if (!ctx || ctx->err) {
    if (ctx) {
      LOG(ERROR) << "Failed to create Redis context: " << ctx->err;
    }
    else {
      LOG(ERROR) << "Failed to allocate Redis context";
    }

    std::abort();
  }

  redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(ctx, "INCR msg_seq_id"));
  if (!reply) {
    LOG(ERROR) << "Failed to send Redis command: " << ctx->err;
    std::abort();
  }

  assert(reply->type == REDIS_REPLY_INTEGER && "returned data isn't an integer");
  assert(reply->integer >= 0 && "returned id is negative");
  assert(reply->integer < std::numeric_limits<uint16_t>::max());

  await_consensus(ctx);

  seq_id = reply->integer;
  last_alloc_seq_num = 0;

  LOG(INFO) << "Allocated id " << seq_id;
}

// --
void sequence_numbers::await_consensus(void *context) {
  redisContext *ctx = reinterpret_cast<redisContext *>(context);

  int quorum = read_variable<int>("MSG_QUORUM", 0);
  if (quorum == 0) {
    return;
  }

  redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(ctx, "WAIT %d 0", quorum));
  if (!reply) {
    LOG(ERROR) << "Failed to send Redis command: " << ctx->err;
    std::abort();
  }

  assert(reply->type == REDIS_REPLY_INTEGER && "returned data isn't an integer");

  if (reply->integer != quorum) {
    LOG(ERROR) << "Failed to reach consensus";
    std::abort();
  }
}

// --
uint64_t sequence_numbers::alloc() {
  pending_seq_nums++;
  return last_alloc_seq_num + pending_seq_nums;
}

// --
void sequence_numbers::commit(uint64_t num) {
  assert(num == last_alloc_seq_num + pending_seq_nums && "trying to jump ahead");
  last_alloc_seq_num = num;
  pending_seq_nums = 0;
}

// --
void sequence_numbers::commit(uint16_t id, uint64_t num) {
  rx_seq_nums[id] = num;
}

// --
uint16_t sequence_numbers::local_id() const {
  return seq_id;
}

// --
int sequence_numbers::check(uint16_t seq_id, uint64_t seq_num) const {
  uint64_t last_seq_num = 0;

  auto iter = rx_seq_nums.find(seq_id);
  if (iter != end(rx_seq_nums)) {
    last_seq_num = iter->second;
  }

  return seq_num - last_seq_num;
}
