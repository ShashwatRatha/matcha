#pragma once

#include <cstdint>
#include <unordered_map>

#include "itch-structs.hpp"
#include "ladder.hpp"
#include "memManage.hpp"
#include "orders.hpp"

struct TradeEvent {
  uint32_t matchPrice;
  uint32_t matchQty;
  uint64_t bidOrderID;
  uint64_t askOrderID;
};

class OrderBook {
 private:
  // @members
  MemManager mMem;
  PriceLadder mSells;
  PriceLadder mBuys;
  std::unordered_map<uint64_t, uint32_t> mIDtoOffsets;

  // @handlers
  void handleAdd(const ITCHStructs::AddOrder& msg);
  void handleAdd(const ITCHStructs::AddOrderMPID& msg);
  void handleSystem(const ITCHStructs::SystemMsg& msg);
  void handleCancel(const ITCHStructs::OrderCancelMsg& msg);
  void handleReplace(const ITCHStructs::OrderReplaceMsg& msg);
  void handleDlt(const ITCHStructs::OrderDltMsg& msg);

  // @internals
  void match(Order& incoming);

 public:
  explicit OrderBook(uint32_t base);
  OrderBook(OrderBook&&) = delete;
  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(OrderBook&&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

  void consumeMsg(const ITCHMsg&);
};
