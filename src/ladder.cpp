#include "ladder.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "constants.hpp"
#include "levels.hpp"
#include "memManage.hpp"

static inline constexpr auto priceIdx = [](uint32_t p1, uint32_t p2) {
  return ((int32_t(p1) - int32_t(p2)) / int32_t(TICK_SIZE) +
          int32_t(HALF_WIDTH));
};

PriceLadder::PriceLadder(Side s, uint32_t base)
    : mType(s), mBasePrice(base), mBestPrice(INVALID_IDX) {
  mLadder = new PriceLevel[2 * HALF_WIDTH + 1];
}

PriceLadder::~PriceLadder() {
  if (mLadder) delete[] mLadder, mLadder = nullptr;
}

uint32_t PriceLadder::bestPrice() const { return mBestPrice; }
bool PriceLadder::isEmpty() const { return mBestPrice == INVALID_IDX; }

PriceLevel& PriceLadder::bestLevel() {
  return mLadder[priceIdx(mBestPrice, mBasePrice)];
}

bool PriceLadder::emplaceOrder(uint32_t ndx, MemManager& mem) {
  auto order = mem.getOrder(ndx);
  if (!order) return false;
  int32_t price = order->orderPrice;
  auto slot = priceIdx(price, mBasePrice);
  if (slot < 0 || slot > 2 * HALF_WIDTH) {
    std::cerr << "Price " << price << " does not fit in ladder.\n";
    return false;
  }
  auto& level = mLadder[slot];

  order->nextOrder = INVALID_IDX, order->prevOrder = INVALID_IDX;

  if (level.newestOrderRefNum == INVALID_IDX) {
    level.oldestOrderRefNum = ndx;
    level.newestOrderRefNum = ndx;
  } else {
    auto oldOrderID = level.newestOrderRefNum;
    auto oldOrder = mem.getOrder(oldOrderID);
    if (!oldOrder) return false;
    oldOrder->nextOrder = ndx;
    order->prevOrder = oldOrderID;
    level.newestOrderRefNum = ndx;
  }

  level.totalShares += order->orderShares;

  if (mBestPrice == INVALID_IDX)
    mBestPrice = price;
  else if (mType == Side::BUY && price > mBestPrice)
    mBestPrice = price;
  else if (mType == Side::SELL && price < mBestPrice)
    mBestPrice = price;

  return true;
}

bool PriceLadder::removeOrder(uint32_t ndx, MemManager& mem) {
  auto order = mem.getOrder(ndx);
  if (!order) return false;
  int32_t price = order->orderPrice;
  auto slot = priceIdx(price, mBasePrice);
  if (slot < 0 || slot > 2 * HALF_WIDTH) {
    std::cerr << "DEBUG slot=" << slot << " price=" << price
              << " base=" << mBasePrice << "\n";
    std::cerr << "Price " << price << " does not fit in ladder\n";
    return false;
  }
  auto& level = mLadder[slot];
  auto prevOrderID = order->prevOrder, nextOrderID = order->nextOrder;

  if (prevOrderID == INVALID_IDX && nextOrderID == INVALID_IDX) {
    level.oldestOrderRefNum = level.newestOrderRefNum = INVALID_IDX;
  } else if (prevOrderID == INVALID_IDX) {
    auto newHead = mem.getOrder(nextOrderID);
    if (!newHead) return false;
    newHead->prevOrder = INVALID_IDX;
    level.oldestOrderRefNum = nextOrderID;
  } else if (nextOrderID == INVALID_IDX) {
    auto newTail = mem.getOrder(prevOrderID);
    if (!newTail) return false;
    newTail->nextOrder = INVALID_IDX;
    level.newestOrderRefNum = prevOrderID;
  } else {
    auto prevOrder = mem.getOrder(prevOrderID),
         nextOrder = mem.getOrder(nextOrderID);
    if (!prevOrder || !nextOrder) return false;
    prevOrder->nextOrder = nextOrderID;
    nextOrder->prevOrder = prevOrderID;
  }

  level.totalShares -= order->orderShares;

  // best price updation
  if (!isEmpty() && bestLevel().isEmpty()) {
    if (mType == Side::SELL) {
      int32_t saved = priceIdx(mBestPrice, mBasePrice);
      mBestPrice = INVALID_IDX;
      for (int32_t p = saved; p <= 2 * HALF_WIDTH; p++)
        if (!mLadder[p].isEmpty()) {
          mBestPrice = (p - HALF_WIDTH) * TICK_SIZE + mBasePrice;
          break;
        }
    }

    if (mType == Side::BUY) {
      int32_t saved = priceIdx(mBestPrice, mBasePrice);
      mBestPrice = INVALID_IDX;
      for (int32_t p = saved; p >= 0; p--)
        if (!mLadder[p].isEmpty()) {
          mBestPrice = (p - HALF_WIDTH) * TICK_SIZE + mBasePrice;
          break;
        }
    }
  }

  return true;
}
