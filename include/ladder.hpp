#pragma once

#include <cstdint>

#include "levels.hpp"
#include "memManage.hpp"

inline constexpr int32_t HALF_WIDTH = 10000;

enum class Side { BUY, ASK, NONE };

class PriceLadder {
 private:
  Side mType = Side::NONE;
  uint32_t mBasePrice = 0;
  uint32_t mBestPrice = 0;
  PriceLevel* mLadder = nullptr;

 public:
  PriceLadder(Side s, uint32_t base);
  ~PriceLadder();

  uint32_t bestPrice() const;
  bool isEmpty() const;

  PriceLevel& bestLevel();
  bool emplaceOrder(uint32_t ndx, MemManager& mem);
  bool removeOrder(uint32_t ndx, MemManager& mem);
};
