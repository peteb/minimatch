// -*- c++ -*-

#ifndef _UTILS_TIMING_H
#define _UTILS_TIMING_H

#include <functional>
#include <chrono>
#include <ratio>
#include <glog/logging.h>
#include <cstdint>
#include <limits>

/*
 * Measures the run-time of some code and averages.
 */
template<int WindowSize, int Skip, typename BufferType>
class timing {
public:
  // --
  timing(const char *name)
    : window()
    , pos(0)
    , skip_pos(0)
    , name(name)
  {
    sum = 0;
    sum_delta = 0;
    t_last = std::chrono::high_resolution_clock::now();
  }

  // --
  template<typename T>
  T benchmark(const std::function<T()> &fun) {
    if (Skip != 0) {
      if (skip_pos++ < Skip) {
        return fun();
      }
      else {
        skip_pos = 0;
      }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    fun();
    auto t2 = std::chrono::high_resolution_clock::now();
    BufferType diff = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    uint64_t delta = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t_last).count();
    t_last = t2;

    auto max_value = std::numeric_limits<BufferType>::max();
    if (delta > max_value) {
      delta = max_value;
      LOG(WARNING) << "Capping value.";
    }

    if (diff < 1) {
      return;
    }

    std::pair<BufferType, BufferType> old_value = window[pos];
    window[pos] = std::make_pair(diff, delta);
    sum = sum - old_value.first + diff;
    sum_delta = sum_delta - old_value.second + delta;

    if (pos == WindowSize - 1) {
      LOG(INFO) << name << ": avg_diff=" << sum / WindowSize << " avg_delta=" << sum_delta / WindowSize;
      pos = 0;
    }
    else {
      ++pos;
    }
  }

private:
  uint64_t sum, sum_delta;
  int pos, skip_pos;
  const char *name;
  std::chrono::high_resolution_clock::time_point t_last;

  std::pair<BufferType, BufferType> window[WindowSize];
};

/*
 * Discrete periods.
 */
class stream_measure {
public:
  stream_measure(const char *name)
    : name(name)
  {
    period_start = std::chrono::high_resolution_clock::now();
  }

  // --
  void collect(size_t size) {
    auto t = std::chrono::high_resolution_clock::now();
    int diff = std::chrono::duration_cast<std::chrono::milliseconds>(t - period_start).count();

    if (diff >= 1000) {
      report();
      period_start = t;
      period_count = 0;
      period_bytes = 0;
    }

    period_count++;
    period_bytes += size;
  }

private:
  void report() {
    LOG(INFO) << name << ": period_bytes=" << period_bytes << " period_count=" << period_count;
  }

  std::chrono::high_resolution_clock::time_point period_start;
  size_t period_bytes;
  uint64_t period_count;
  const char *name;
};

#endif // !_UTILS_TIMING_H
