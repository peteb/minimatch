// -* c++ -*-

#ifndef _UTILS_MEMORY_H
#define _UTILS_MEMORY_H

#include <memory>

template<typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

#endif // !_UTILS_MEMORY_H
