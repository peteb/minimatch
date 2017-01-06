// -*- c++ -*-

#ifndef _UTILS_TIMING_H
#define _UTILS_TIMING_H

#include <functional>
#include <chrono>
#include <ratio>
#include <glog/logging.h>
#include <cstdint>

template<int WindowSize, int Skip, typename BufferType>
class timing {
public:
  timing(const char *name)
    : window()
    , pos(0)
    , skip_pos(0)
    , name(name)
  {
  }

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
    int diff = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

    int old_diff = window[pos];
    window[pos] = diff;
    sum = sum - old_diff + diff;

    if (pos == WindowSize - 1) {
      LOG(INFO) << name << ": avgDiff=" << sum / WindowSize;
      pos = 0;
    }
    else {
      ++pos;
    }
  }

private:
  int pos, skip_pos;
  uint64_t sum;
  BufferType window[WindowSize];
  const char *name;
};

#endif // !_UTILS_TIMING_H
