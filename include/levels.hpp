#pragma once

#include <cstdint>

#include "constants.hpp"

struct PriceLevel {
  uint32_t levelPrice = 0;
  uint32_t totalShares = 0;
  uint32_t oldestOrderRefNum = INVALID_IDX;
  uint32_t newestOrderRefNum = INVALID_IDX;

  bool isEmpty() const { return oldestOrderRefNum == INVALID_IDX; }
};
