#pragma once

#include <cstdint>
#include <variant>

// inline swap helpers
constexpr auto bs16 = [](uint16_t s) { return __builtin_bswap16(s); };
constexpr auto bs32 = [](uint32_t s) { return __builtin_bswap32(s); };
constexpr auto bs64 = [](uint64_t s) { return __builtin_bswap64(s); };

namespace ITCHStructs {
struct AddOrder {
  uint8_t msgType = 'A';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint8_t bsInd;
  uint32_t shares;
  uint8_t stock[8];
  uint32_t price;
} __attribute__((packed));

struct AddOrderMPID {
  uint8_t msgType = 'F';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint8_t bsInd;
  uint32_t shares;
  uint8_t stock[8];
  uint32_t price;
  uint8_t attr[4];
} __attribute__((packed));

struct SystemMsg {
  uint8_t msgType = 'S';
  uint16_t stockLocate = 0;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint8_t eventCode;
} __attribute__((packed));

struct OrderCancelMsg {
  uint8_t msgType = 'X';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint32_t cancelShares;
} __attribute__((packed));

struct OrderDltMsg {
  uint8_t msgType = 'D';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
} __attribute__((packed));

struct OrderReplaceMsg {
  uint8_t msgType = 'U';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orgORN;
  uint64_t newORN;
  uint32_t newShares;
  uint32_t newPrice;
} __attribute__((packed));

struct NullTp {};
};  // namespace ITCHStructs

using ITCHMsg =
    std::variant<ITCHStructs::AddOrder, ITCHStructs::AddOrderMPID,
                 ITCHStructs::OrderCancelMsg, ITCHStructs::OrderDltMsg,
                 ITCHStructs::OrderReplaceMsg, ITCHStructs::SystemMsg,
                 ITCHStructs::NullTp>;
