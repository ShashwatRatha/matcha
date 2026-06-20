#pragma once

#include <cstdint>

const uint32_t INVALID_IDX = 0xffffffff;

struct PriceLevel {
  uint32_t price = 0;
  uint32_t totalVolume = 0;
  uint32_t oldestOrderID = INVALID_IDX;
  uint32_t newestOrderID = INVALID_IDX;

  bool isEmpty() const { return oldestOrderID == INVALID_IDX; }
};
