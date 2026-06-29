#pragma once

#include <cstdint>

void generateBinFile();

namespace MsgStructs {
struct AddOrderWithoutMPID {
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

struct AddOrderWithMPID {
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

struct OrderExecutedMsg {
  uint8_t msgType = 'E';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint32_t execShares;
  uint64_t matchNum;
} __attribute__((packed));

struct OrderExecutedWithPriceMsg {
  uint8_t msgType = 'C';
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint32_t execShares;
  uint64_t matchNum;
  uint8_t printable;
  uint32_t execPrice;
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
};  // namespace MsgStructs
