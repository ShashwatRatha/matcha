#include "order-book.hpp"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <variant>

#include "constants.hpp"
#include "itch-structs.hpp"
#include "ladder.hpp"
#include "levels.hpp"
#include "orders.hpp"

OrderBook::OrderBook(uint32_t base)
    : mMem(),
      mSells(Side::SELL, base),
      mBuys(Side::BUY, base),
      mIDtoOffsets(){};

static uint32_t tradeIdx = 0;

static void logTrade(const TradeEvent& trade) {
  std::cout << "Trade " << ++tradeIdx << ":\n"
            << "Buy OrderId: " << trade.bidOrderID << "\n"
            << "Ask OrderID: " << trade.askOrderID << "\n"
            << "Price: " << trade.matchPrice << "\n"
            << "Quantity: " << trade.matchQty << "\n\n";
}

// struct to allow proper symbol resolution adhering to polymorphism
template <typename... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

// template instantiation deduction hint
template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void OrderBook::consumeMsg(const ITCHMsg& msg) {
  std::visit(
      overloaded{
          [](const ITCHStructs::NullTp& msg) {
            std::cerr << "Invalid msg type.\n";
          },
          [this](const ITCHStructs::AddOrder& msg) { handleAdd(msg); },
          [this](const ITCHStructs::AddOrderMPID& msg) { handleAdd(msg); },
          [this](const ITCHStructs::SystemMsg& msg) { handleSystem(msg); },
          [this](const ITCHStructs::OrderDltMsg& msg) { handleDlt(msg); },
          [this](const ITCHStructs::OrderCancelMsg& msg) { handleCancel(msg); },
          [this](const ITCHStructs::OrderReplaceMsg& msg) {
            handleReplace(msg);
          }},
      msg);
}

void OrderBook::handleAdd(const ITCHStructs::AddOrderMPID& msg) {
  ITCHStructs::AddOrder message;
  std::memcpy(&message, &msg, sizeof(message));
  handleAdd(message);
}

void OrderBook::handleAdd(const ITCHStructs::AddOrder& msg) {
  auto shares = bs32(msg.shares);
  auto price = bs32(msg.price);
  auto orderRefNum = bs64(msg.orderRefNum);

  if (mIDtoOffsets.find(orderRefNum) != mIDtoOffsets.end()) {
    std::cerr << "Tried to add existing order to book (Ref Num: " << orderRefNum
              << ")\n";
    return;
  }

  if (shares == 0) {
    std::cerr << "Cannot add order with 0 shares (MsgTrackNum: #"
              << bs16(msg.trackNum) << ")\n";
    return;
  }

  if (price % TICK_SIZE != 0) {
    std::cerr << "Attempted AddOrder (Ref Num: " << orderRefNum
              << ") has price " << price
              << " which is not tick-aligned. Skipping add for this order.\n";
    return;
  }

  auto arenaIdx = mMem.allocateOrder();
  if (arenaIdx >= INVALID_IDX) {
    std::cerr << "OrderBook arena is full; cannot allocate order with Ref Num: "
              << orderRefNum << "\n";
    return;
  }

  auto order = mMem.getOrder(arenaIdx);
  order->orderRefNum = orderRefNum;
  order->orderPrice = price;
  order->orderQueue = msg.bsInd;
  order->orderShares = shares;
  order->prevOrder = order->nextOrder = INVALID_IDX;

  match(*order);
  if (order->orderShares > 0) {
    if (order->orderQueue == 'B')
      mBuys.emplaceOrder(arenaIdx, mMem);
    else if (order->orderQueue == 'S')
      mSells.emplaceOrder(arenaIdx, mMem);
    else {
      std::cerr << "Invalid queue ID: " << order->orderQueue << "\n";
      return;
    }
    mIDtoOffsets[order->orderRefNum] = arenaIdx;
  }

  if (order->orderShares <= 0) mMem.freeOrder(arenaIdx);
}

void OrderBook::handleCancel(const ITCHStructs::OrderCancelMsg& msg) {
  auto orderRefNum = bs64(msg.orderRefNum);
  auto shares = bs32(msg.cancelShares);

  if (mIDtoOffsets.find(orderRefNum) == mIDtoOffsets.end()) {
    std::cerr << "Tried to cancel non-existing order. Ref Num: " << orderRefNum
              << "\n";
    return;
  }

  auto arenaIdx = mIDtoOffsets[orderRefNum];
  auto order = mMem.getOrder(arenaIdx);

  if (order->orderShares <= shares) {
    if (order->orderShares < shares)
      std::cerr << "Order #" << orderRefNum << " does not have " << shares
                << "shares. Zeroing out the shares and removing the order.\n";
    order->orderShares = 0;
    if (order->orderQueue == 'B')
      mBuys.removeOrder(arenaIdx, mMem);
    else
      mSells.removeOrder(arenaIdx, mMem);
    mIDtoOffsets.erase(orderRefNum);
    mMem.freeOrder(arenaIdx);

    return;
  }

  order->orderShares -= shares;
}

void OrderBook::handleDlt(const ITCHStructs::OrderDltMsg& msg) {
  auto orderRefNum = bs64(msg.orderRefNum);
  if (mIDtoOffsets.find(orderRefNum) == mIDtoOffsets.end()) {
    std::cerr << "Tried to delete a non-existing order (Ref Num: "
              << orderRefNum << ")\n";
    return;
  }

  auto arenaIdx = mIDtoOffsets[orderRefNum];
  auto queue = mMem.getOrder(arenaIdx)->orderQueue;

  if (queue == 'S')
    mSells.removeOrder(arenaIdx, mMem);
  else
    mBuys.removeOrder(arenaIdx, mMem);
  mIDtoOffsets.erase(orderRefNum);
  mMem.freeOrder(arenaIdx);
}

void OrderBook::handleReplace(const ITCHStructs::OrderReplaceMsg& msg) {
  auto orgORN = bs64(msg.orgORN);

  if (mIDtoOffsets.find(orgORN) == mIDtoOffsets.end()) {
    std::cerr << "Tried to replace non-existing order #" << orgORN << "\n";
    return;
  }

  auto queue = mMem.getOrder(mIDtoOffsets[orgORN])->orderQueue;

  handleDlt(ITCHStructs::OrderDltMsg{.msgType = 'D',
                                     .stockLocate = 0,
                                     .trackNum = 0,
                                     .timestamp = {},
                                     .orderRefNum = msg.orgORN});
  handleAdd(ITCHStructs::AddOrder{.msgType = 'A',
                                  .stockLocate = 0,
                                  .trackNum = 0,
                                  .timestamp = {},
                                  .orderRefNum = msg.newORN,
                                  .bsInd = queue,
                                  .shares = msg.newShares,
                                  .stock = {},
                                  .price = msg.newPrice});
}

void OrderBook::handleSystem(const ITCHStructs::SystemMsg& msg) {
  uint64_t time = 0;
  uint8_t i = 0;
  while (i < 6) time = (time << 8) | msg.timestamp[i++];

  auto eventCode = msg.eventCode;
  std::string message =
      (eventCode == 'O')   ? "Market Hours Start."
      : (eventCode == 'S') ? "System Starts."
      : (eventCode == 'Q') ? "Retail Begins."
      : (eventCode == 'M') ? "Retail Ends."
      : (eventCode == 'E') ? "System Ends."
      : (eventCode == 'C') ? "Market Hours End."
                           : "Illegal messageType " + std::to_string(eventCode);

  std::cout << "System Msg at Time: " << time << " -> " << message << "\n";
}

void OrderBook::match(Order& incoming) {
  auto incomingSide = incoming.orderQueue;
  if (incomingSide == 'S') {
    auto remainingQty = incoming.orderShares, price = incoming.orderPrice;
    while (remainingQty > 0 && !mBuys.isEmpty() && price <= mBuys.bestPrice()) {
      auto& level = mBuys.bestLevel();
      while (!level.isEmpty()) {
        auto headIdx = level.oldestOrderRefNum;
        auto headOrder = mMem.getOrder(headIdx);
        auto matchedQty = std::min(remainingQty, headOrder->orderShares);
        remainingQty -= matchedQty, headOrder->orderShares -= matchedQty;
        TradeEvent event = {.matchPrice = headOrder->orderPrice,
                            .matchQty = matchedQty,
                            .bidOrderID = headOrder->orderRefNum,
                            .askOrderID = incoming.orderRefNum};
        logTrade(event);
        if (headOrder->orderShares == 0) {
          mIDtoOffsets.erase(headOrder->orderRefNum);
          mBuys.removeOrder(headIdx, mMem);
          mMem.freeOrder(headIdx);
        }
        if (remainingQty == 0) break;
      }
    }

    incoming.orderShares = remainingQty;
  }

  if (incomingSide == 'B') {
    auto remainingQty = incoming.orderShares, price = incoming.orderPrice;
    while (remainingQty > 0 && !mSells.isEmpty() &&
           price >= mSells.bestPrice()) {
      auto& level = mSells.bestLevel();
      while (!level.isEmpty()) {
        auto headIdx = level.oldestOrderRefNum;
        auto headOrder = mMem.getOrder(headIdx);
        auto matchedQty = std::min(remainingQty, headOrder->orderShares);
        remainingQty -= matchedQty, headOrder->orderShares -= matchedQty;
        TradeEvent event = {.matchPrice = headOrder->orderPrice,
                            .matchQty = matchedQty,
                            .bidOrderID = incoming.orderRefNum,
                            .askOrderID = headOrder->orderRefNum};
        logTrade(event);
        if (headOrder->orderShares == 0) {
          mIDtoOffsets.erase(headOrder->orderRefNum);
          mSells.removeOrder(headIdx, mMem);
          mMem.freeOrder(headIdx);
        }
        if (remainingQty == 0) break;
      }
    }

    incoming.orderShares = remainingQty;
  }
}
