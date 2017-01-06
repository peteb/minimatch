// -*- c++ -*-

#ifndef _UTILS_BLOCKING_QUEUE_H
#define _UTILS_BLOCKING_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <deque>

template<typename T>
class blocking_queue {
public:
  void push(T &&value) {
    {
      std::lock_guard<std::mutex> lock(m);
      queue.push_front(std::move(value));
    }

    signal.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(m);
    signal.wait(lock, [=]{ return !queue.empty(); });

    T retval(std::move(queue.back()));
    queue.pop_back();
    return retval;
  }

private:
  std::deque<T> queue;
  std::mutex m;
  std::condition_variable signal;
};

#endif // !_UTILS_BLOCKING_QUEUE_H
