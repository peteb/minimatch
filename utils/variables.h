// -*- c++ -*-

#ifndef _UTILS_VARIABLES_H
#define _UTILS_VARIABLES_H

#include <string>
#include <cstdlib>
#include <algorithm>
#include <cstring>

// --
template<typename T>
inline T convert(const char *value);

template<>
inline std::string convert(const char *value) {
  return std::string(value);
}

template<>
inline bool convert(const char *value) {
  std::string lcased = value;
  std::transform(begin(lcased), end(lcased), begin(lcased), ::tolower);

  return lcased == "1" || lcased == "true" || lcased == "yes";
}

// --
template<typename T>
inline T read_variable(const char *name, const T &default_value = T()) {
  char *envvar = getenv(name);
  if (envvar && strlen(envvar) > 0) {
    return convert<T>(envvar);
  }
  else {
    return default_value;
  }
}

#endif // !_UTILS_VARIABLES_H
