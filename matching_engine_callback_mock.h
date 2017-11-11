// -*- c++ -*-

#ifndef _MATCHING_ENGINE_CALLBACK_MOCK_H
#define _MATCHING_ENGINE_CALLBACK_MOCK_H

#include "framework/services.h"

class MatchingEngineCallbackMock {
public:
  void register_callback(const char *ins_id, matching_engine_t *service) {

  }

  static matching_engine_callback_t callback;
};

#endif // !_MATCHING_ENGINE_CALLBACK_MOCK_H
