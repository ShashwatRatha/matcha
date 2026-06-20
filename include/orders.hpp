#pragma once

#include <cstdint>

constexpr uint32_t INVALID_ORDER = 0xffffffff;

struct Order {
  uint32_t orderID = INVALID_ORDER;
  uint32_t orderPrice = 0;
  uint32_t orderQty = 0;
  uint32_t nextOrder = INVALID_ORDER;
  uint32_t prevOrder = INVALID_ORDER;
  char orderType;
} __attribute__((packed));
