#include "order-book.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

#include "levels.hpp"
#include "orders.hpp"
#include "parser.hpp"

static void prettyPrint(const std::vector<TradeEvent>& trades) {
  auto i = 1;
  for (const auto& trade : trades)
    std::cout << "Trade " << i++ << ":\n"
              << "Buy OrderId: " << trade.bidOrderID << "\n"
              << "Ask OrderID: " << trade.askOrderID << "\n"
              << "Price: " << trade.matchPrice << "\n"
              << "Quantity: " << trade.matchQty << "\n\n";
}

OrderBook::OrderBook(uint32_t base)
    : mMem(), mAsks(Side::ASK, base), mBuys(Side::BUY, base), mIDtoOffsets() {}

bool OrderBook::consumeOrder(const Parser::ParsedOutput& p) {
  switch (p.action) {
    case 'A':
      return addOrder(p);
    case 'R':
      return replaceOrder(p);
    case 'C':
      return cancelOrder(p.orderID);
  }

  return false;
}

bool OrderBook::addOrder(const Parser::ParsedOutput& p) {
  if (mIDtoOffsets.find(p.orderID) != mIDtoOffsets.end()) return false;

  auto orderPoolIdx = mMem.allocateOrder();
  if (orderPoolIdx == INVALID_IDX) return false;

  auto order = mMem.getOrder(orderPoolIdx);
  order->orderID = p.orderID;
  order->orderPrice = p.orderPrice;
  order->orderQty = p.orderQty;
  order->orderType = p.side;
  order->nextOrder = order->prevOrder = INVALID_IDX;

  std::vector<TradeEvent> matchedTrades;
  match(matchedTrades, *order);

  if (order->orderQty > 0) {
    mIDtoOffsets[order->orderID] = orderPoolIdx;
    if (order->orderType == 'A')
      mAsks.emplaceOrder(orderPoolIdx, mMem);
    else if (order->orderType == 'B')
      mBuys.emplaceOrder(orderPoolIdx, mMem);
  } else {
    mMem.freeOrder(orderPoolIdx);
  }

  prettyPrint(matchedTrades);

  return true;
}

bool OrderBook::cancelOrder(uint32_t id) {
  if (mIDtoOffsets.find(id) == mIDtoOffsets.end()) return false;

  auto orderPoolIdx = mIDtoOffsets[id];
  auto order = mMem.getOrder(orderPoolIdx);

  if (order->orderType == 'A')
    mAsks.removeOrder(orderPoolIdx, mMem);
  else
    mBuys.removeOrder(orderPoolIdx, mMem);

  mIDtoOffsets.erase(order->orderID);
  mMem.freeOrder(orderPoolIdx);

  return true;
}

bool OrderBook::replaceOrder(const Parser::ParsedOutput& p) {
  if (mIDtoOffsets.find(p.orderID) == mIDtoOffsets.end()) return false;

  cancelOrder(p.orderID);
  return addOrder(p);
}

void OrderBook::match(std::vector<TradeEvent>& trades, Order& incoming) {
  auto incomingSide = incoming.orderType;
  if (incomingSide == 'A') {
    auto remainingQty = incoming.orderQty, price = incoming.orderPrice;
    while (remainingQty > 0 && !mBuys.isEmpty() && price <= mBuys.bestPrice()) {
      auto& level = mBuys.bestLevel();
      while (!level.isEmpty()) {
        auto headIdx = level.oldestOrderID;
        auto headOrder = mMem.getOrder(headIdx);
        auto matchedQty = std::min(remainingQty, headOrder->orderQty);
        remainingQty -= matchedQty, headOrder->orderQty -= matchedQty;
        TradeEvent event = {.matchPrice = headOrder->orderPrice,
                            .matchQty = matchedQty,
                            .bidOrderID = headOrder->orderID,
                            .askOrderID = incoming.orderID};
        trades.push_back(event);
        if (headOrder->orderQty == 0) {
          mIDtoOffsets.erase(headOrder->orderID);
          mBuys.removeOrder(headIdx, mMem);
          mMem.freeOrder(headIdx);
        }
        if (remainingQty == 0) break;
      }
    }

    incoming.orderQty = remainingQty;
  }

  if (incomingSide == 'B') {
    auto remainingQty = incoming.orderQty, price = incoming.orderPrice;
    while (remainingQty > 0 && !mAsks.isEmpty() && price >= mAsks.bestPrice()) {
      auto& level = mAsks.bestLevel();
      while (!level.isEmpty()) {
        auto headIdx = level.oldestOrderID;
        auto headOrder = mMem.getOrder(headIdx);
        auto matchedQty = std::min(remainingQty, headOrder->orderQty);
        remainingQty -= matchedQty, headOrder->orderQty -= matchedQty;
        TradeEvent event = {.matchPrice = headOrder->orderPrice,
                            .matchQty = matchedQty,
                            .bidOrderID = incoming.orderID,
                            .askOrderID = headOrder->orderID};
        trades.push_back(event);
        if (headOrder->orderQty == 0) {
          mIDtoOffsets.erase(headOrder->orderID);
          mAsks.removeOrder(headIdx, mMem);
          mMem.freeOrder(headIdx);
        }
        if (remainingQty == 0) break;
      }
    }

    incoming.orderQty = remainingQty;
  }
}
