#include "ladder.hpp"

#include <cstdint>
#include <cstdlib>

#include "levels.hpp"
#include "memManage.hpp"

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
  return mLadder[mBestPrice - mBasePrice + HALF_WIDTH];
}

bool PriceLadder::emplaceOrder(uint32_t ndx, MemManager& mem) {
  auto order = mem.getOrder(ndx);
  if (!order) return false;
  auto price = order->orderPrice;
  auto& level = mLadder[price - mBasePrice + HALF_WIDTH];

  order->nextOrder = INVALID_IDX, order->prevOrder = INVALID_IDX;

  if (level.newestOrderID == INVALID_IDX) {
    level.oldestOrderID = ndx;
    level.newestOrderID = ndx;
  } else {
    auto oldOrderID = level.newestOrderID;
    auto oldOrder = mem.getOrder(oldOrderID);
    if (!oldOrder) return false;
    oldOrder->nextOrder = ndx;
    order->prevOrder = oldOrderID;
    level.newestOrderID = ndx;
  }

  level.totalVolume += order->orderQty;

  if (mBestPrice == INVALID_IDX)
    mBestPrice = price;
  else if (mType == Side::BUY && price > mBestPrice)
    mBestPrice = price;
  else if (mType == Side::ASK && price < mBestPrice)
    mBestPrice = price;

  return true;
}

bool PriceLadder::removeOrder(uint32_t ndx, MemManager& mem) {
  auto order = mem.getOrder(ndx);
  if (!order) return false;
  auto price = order->orderPrice;
  auto& level = mLadder[price - mBasePrice + HALF_WIDTH];
  auto prevOrderID = order->prevOrder, nextOrderID = order->nextOrder;

  if (prevOrderID == INVALID_IDX && nextOrderID == INVALID_IDX) {
    level.oldestOrderID = level.newestOrderID = INVALID_IDX;
  } else if (prevOrderID == INVALID_IDX) {
    auto newHead = mem.getOrder(nextOrderID);
    if (!newHead) return false;
    newHead->prevOrder = INVALID_IDX;
    level.oldestOrderID = nextOrderID;
  } else if (nextOrderID == INVALID_IDX) {
    auto newTail = mem.getOrder(prevOrderID);
    if (!newTail) return false;
    newTail->nextOrder = INVALID_IDX;
    level.newestOrderID = prevOrderID;
  } else {
    auto prevOrder = mem.getOrder(prevOrderID),
         nextOrder = mem.getOrder(nextOrderID);
    if (!prevOrder || !nextOrder) return false;
    prevOrder->nextOrder = nextOrderID;
    nextOrder->prevOrder = prevOrderID;
  }

  level.totalVolume -= order->orderQty;

  // best price updation
  if (bestLevel().isEmpty()) {
    if (mType == Side::ASK) {
      int32_t saved = mBestPrice;
      mBestPrice = INVALID_IDX;
      for (int32_t p = saved - mBasePrice + HALF_WIDTH; p <= 2 * HALF_WIDTH;
           p++)
        if (!mLadder[p].isEmpty()) {
          mBestPrice = p + mBasePrice - HALF_WIDTH;
          break;
        }
    }

    if (mType == Side::BUY) {
      int32_t saved = mBestPrice;
      mBestPrice = INVALID_IDX;
      for (int32_t p = saved - mBasePrice + HALF_WIDTH; p >= 0; p--)
        if (!mLadder[p].isEmpty()) {
          mBestPrice = p + mBasePrice - HALF_WIDTH;
          break;
        }
    }
  }

  return true;
}
