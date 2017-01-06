// -*- c++ -*-

#ifndef _UTILS_THREAD_H
#define _UTILS_THREAD_H

#include <thread>
#include <mutex>
#include <map>
#include <pthread.h>
#include <atomic>
#include <sched.h>
#include <glog/logging.h>

#define MAX_CPU_COUNT 64

// Too lazy to add a TU just for these guys
__attribute__((weak)) std::atomic<int> next_cpu;
__attribute__((weak)) std::once_flag cpuset_flag;
__attribute__((weak)) cpu_set_t proc_affinity;
__attribute__((weak)) std::map<pthread_t, std::string> thread_names;
__attribute__((weak)) std::mutex name_m;

// --
inline int alloc_cpu() {
  std::call_once(cpuset_flag, []{
    CPU_ZERO(&proc_affinity);
    sched_getaffinity(getpid(), sizeof(proc_affinity), &proc_affinity);
    LOG(INFO) << "Affinity: num_cpu=" << CPU_COUNT(&proc_affinity);
    next_cpu = 0;
  });

  for (int i = next_cpu; i < MAX_CPU_COUNT; ++i) {
    if (CPU_ISSET(i, &proc_affinity)) {
      next_cpu = i + 1;
      return i;
    }
  }

  LOG(ERROR) << "No CPU cores left.";
  std::abort();
}

// --
inline const char *thread_name(pthread_t tid) {
  auto it = thread_names.find(tid);
  if (it != end(thread_names)) {
    return it->second.c_str();
  }
  else {
    return "unnamed";
  }
}

// --
inline void pin_thread(pthread_t tid, int cpu) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);

  int ret = pthread_setaffinity_np(tid, sizeof(cpuset), &cpuset);
  if (ret != 0) {
    LOG(ERROR) << "Failed to set thread affinity: " << ret;
    std::abort();
  }

  std::lock_guard<std::mutex> lock(name_m);
  LOG(INFO) << "Pinned thread " << thread_name(tid) << " [" << tid << "] to cpu " << cpu;
}

// --
inline void pin_current_thread() {
  pin_thread(pthread_self(), alloc_cpu());
}

// --
template<typename... Args>
std::thread cpu_thread(const char *name, Args&&... args) {
  int cpu = alloc_cpu();
  std::thread t(std::forward<Args>(args)...);

  {
    std::lock_guard<std::mutex> lock(name_m);
    thread_names[t.native_handle()] = name;
  }

  pin_thread(t.native_handle(), cpu);
  return t;
}

// --
inline void set_thread_name(const char *name) {
  std::lock_guard<std::mutex> lock(name_m);
  thread_names.insert({pthread_self(), {name}});
}

#endif // !_UTILS_THREAD_H
