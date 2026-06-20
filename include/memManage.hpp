#pragma once

#include "constants.hpp"
#include "orders.hpp"

class MemManager {
 public:
  MemManager();
  ~MemManager();

  MemManager(const MemManager&) = delete;
  MemManager& operator=(const MemManager&) = delete;
  MemManager(MemManager&&) = delete;
  MemManager& operator=(MemManager&&) = delete;

  uint32_t allocateOrder(void);
  Order* getOrder(const uint32_t& ndx) const;
  void freeOrder(const uint32_t& ndx);

 private:
  Order* mArena;
  uint32_t* mFreeList;
  int32_t mFront;
  int32_t mFreeIdx;
  uint32_t mCapacity;
};
