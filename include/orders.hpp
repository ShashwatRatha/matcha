#pragma once

#include <cstdint>

extern const uint32_t INVALID_IDX;

struct Order {
  uint32_t orderID = INVALID_IDX;
  uint32_t orderPrice = 0;
  uint32_t orderQty = 0;
  uint32_t nextOrder = INVALID_IDX;
  uint32_t prevOrder = INVALID_IDX;
  char orderType;
} __attribute__((packed));
