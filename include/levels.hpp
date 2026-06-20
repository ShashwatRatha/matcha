#pragma once

#include <cstdint>

#include "constants.hpp"

struct PriceLevel {
  uint32_t price = 0;
  uint32_t totalVolume = 0;
  uint32_t oldestOrderID = INVALID_IDX;
  uint32_t newestOrderID = INVALID_IDX;

  bool isEmpty() const { return oldestOrderID == INVALID_IDX; }
};
