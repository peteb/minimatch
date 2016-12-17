#include "services.h"

#include <map>
#include <string>
#include <glog/logging.h>
#include <cstdlib>

std::map<std::string, void *> descriptors;

void *find_service(const char *name) {
  auto iter = descriptors.find(std::string(name));
  if (iter == descriptors.end()) {
    LOG(ERROR) << "Failed to find service '" << name << "'";
    std::exit(1);
  }

  return iter->second;
}

void register_service(const char *name, void *descriptor) {
  const std::string name_str(name);

  if (descriptors.find(name_str) != descriptors.end()) {
    LOG(ERROR) << "Service '" << name << "' already registered";
    std::exit(1);
  }

  descriptors.insert({name_str, descriptor});
}

void unregister_service(const char *name, void *descriptor) {
  auto iter = descriptors.find({name});
  if (iter != descriptors.end()) {
    if (iter->second != descriptor) {
      LOG(ERROR) << "Descriptor for service '" << name << "' isn't matching";
      std::abort();
    }

    descriptors.erase(iter);
  }
}
