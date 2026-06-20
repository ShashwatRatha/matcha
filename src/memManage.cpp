#include "memManage.hpp"

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

MemManager::MemManager() : mCapacity(1024), mFreeIdx(-1), mFront(-1) {
  mArena = new Order[mCapacity];
  mFreeList = new uint32_t[mCapacity];
}

MemManager::~MemManager() {
  delete[] mArena;
  delete[] mFreeList;
  mArena = nullptr, mFreeList = nullptr;
}

Order* MemManager::getOrder(const uint32_t& ndx) const {
  return (ndx < mCapacity) ? mArena + ndx : nullptr;
}

uint32_t MemManager::allocateOrder() {
  if (mFreeIdx >= 0) return mFreeList[mFreeIdx--];
  if (mFront + 1 < mCapacity) return ++mFront;
  return -1;
}

void MemManager::freeOrder(const uint32_t& ndx) {
  if (mFreeIdx + 1 >= mCapacity)
    throw std::runtime_error("All elements freed. no more frees possible");
  if (ndx >= mCapacity && ndx != -1)
    throw std::runtime_error("attempt to free invalid address");
  mFreeList[++mFreeIdx] = ndx;
}
