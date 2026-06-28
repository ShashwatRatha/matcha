#pragma once

#include <cstdint>
#include <string_view>

// CSV Lines Format:
// MsgType, StockLocate, TrackNum, Timestamp, OrderRefNum (curr or old (for
// replace orders)), New OrderRefNum (only for replace)/matchNum (only for
// Executed order), BuySell/printable/evntcde, Shares, price, stock, attr

namespace Parser {
struct ParsedOutput {
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint64_t matchNumNewORN;
  uint8_t bsPrintEvCd;
  uint32_t shares;
  uint32_t price;
  uint8_t stock[8];
  uint8_t attr[4];
} __attribute__((packed));

bool parse(std::string_view line, ParsedOutput& output);
};  // namespace Parser
