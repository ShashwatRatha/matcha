#pragma once

#include <cstdint>

extern const uint32_t INVALID_IDX;

struct Order {
  uint8_t orderQueue;
  uint64_t orderRefNum = INVALID_IDX;
  uint32_t orderPrice = 0;
  uint32_t orderShares = 0;
  uint32_t nextOrder = INVALID_IDX;
  uint32_t prevOrder = INVALID_IDX;
} __attribute__((packed));
