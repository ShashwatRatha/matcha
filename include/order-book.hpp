#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "itch-structs.hpp"
#include "ladder.hpp"
#include "memManage.hpp"
#include "orders.hpp"

struct TradeEvent {
  uint32_t matchPrice;
  uint32_t matchQty;
  uint32_t bidOrderID;
  uint32_t askOrderID;
};

class OrderBook {
 private:
  MemManager mMem;
  PriceLadder mAsks;
  PriceLadder mBuys;
  std::unordered_map<uint32_t, uint32_t> mIDtoOffsets;
  // bool addOrder(const Parser::ParsedOutput&);
  // bool replaceOrder(const Parser::ParsedOutput&);
  bool cancelOrder(uint32_t id);
  void match(std::vector<TradeEvent>& out, Order& incoming);

 public:
  OrderBook(uint32_t base);

  bool consumeMsg(const ITCHMsg&);
};
