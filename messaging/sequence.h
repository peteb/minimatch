// -*- c++ -*-

#ifndef _MESSAGING_SEQUENCE_H
#define _MESSAGING_SEQUENCE_H

#include <cstdint>
#include <unordered_map>

class sequence_numbers {
public:
  sequence_numbers();

  uint64_t alloc();
  void     commit(uint64_t num);
  void     commit(uint16_t id, uint64_t num);

  uint16_t local_id() const;

  int check(uint16_t seq_id, uint64_t seq_num) const;

private:
  void await_consensus(void *context);

  uint16_t seq_id;
  uint64_t last_alloc_seq_num;
  int pending_seq_nums;
  std::unordered_map<uint16_t, uint64_t> rx_seq_nums;
};

#endif // !_MESSAGING_SEQUENCE_H
