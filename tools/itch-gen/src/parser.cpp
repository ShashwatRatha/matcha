#include "parser.hpp"

#include <cctype>
#include <cstdint>
#include <cstring>

inline bool isValidMsgType(const char c) {
  return (c == 'A' || c == 'S' || c == 'F' || c == 'E' || c == 'C' ||
          c == 'X' || c == 'D' || c == 'U');
}

inline uint16_t be16(const uint16_t p) { return __builtin_bswap16(p); }

inline uint32_t be32(const uint32_t p) { return __builtin_bswap32(p); }

inline uint64_t be64(const uint64_t p) { return __builtin_bswap64(p); }

// CSV Lines Format:
// MsgType, StockLocate, TrackNum, Timestamp, OrderRefNum (curr or old (for
// replace orders)), New OrderRefNum (only for replace)/matchNum (only for
// Executed order), BuySell/printable/evntcde, Shares, price, stock, attr

bool Parser::parse(std::string_view line, ParsedOutput &output) {
  auto ptr = line.data(), end = line.data() + line.size();
  if (ptr == end) return false;

  if (!isValidMsgType(*ptr)) return false;
  auto t = output.msgType = *ptr;
  ptr++;
  if (ptr == end || *ptr != ',') return false;
  ptr++;

  uint16_t stockLocate = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (stockLocate > ((UINT16_MAX - (*ptr - '0')) / 10)) return false;
    stockLocate = 10 * stockLocate + (*ptr - '0');
    ptr++;
  }
  if (ptr == end) return false;
  output.stockLocate = be16(stockLocate);
  ptr++;
  if (ptr == end) return false;

  uint16_t trackNum = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (trackNum > ((UINT16_MAX - (*ptr - '0')) / 10)) return false;
    trackNum = 10 * trackNum + (*ptr - '0');
    ptr++;
  }
  if (ptr == end) return false;
  output.trackNum = be16(trackNum);
  ptr++;
  if (ptr == end) return false;

  uint64_t ts = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (ts > ((UINT64_MAX - (*ptr - '0')) / 10)) return false;
    ts = 10 * ts + (*ptr - '0');
    ptr++;
  }
  if (ptr == end) return false;
  for (int i = 5; i >= 0; i--) {
    output.timestamp[i] = (ts & 0xff);
    ts >>= 8;
  }
  if (ts > 0) return false;
  ptr++;
  if (ptr == end) return false;

  uint64_t orn = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (orn > ((UINT64_MAX - (*ptr - '0')) / 10)) return false;
    orn = 10 * orn + (*ptr - '0');
    ptr++;
  }
  if (t == 'S' && orn != 0) return false;
  if (ptr == end) return false;
  output.orderRefNum = be64(orn);
  ptr++;
  if (ptr == end) return false;

  uint64_t matchNumNewORN = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (matchNumNewORN > ((UINT64_MAX - (*ptr - '0')) / 10)) return false;
    matchNumNewORN = matchNumNewORN * 10 + (*ptr - '0');
    ptr++;
  }
  if ((t != 'E' && t != 'C' && t != 'U') && matchNumNewORN > 0) return false;
  if (ptr == end) return false;
  output.matchNumNewORN = be64(matchNumNewORN);
  ptr++;
  if (ptr == end) return false;

  char ch = output.bsPrintEvCd = (*ptr != ',') ? *ptr : 0;
  ptr += (ch != 0);

  if (t == 'A' || t == 'F') {
    if (ch != 'B' && ch != 'S') return false;
  } else if (t == 'C') {
    if (ch != 'Y' && ch != 'N') return false;
  } else if (t == 'S') {
    if (ch != 'O' && ch != 'S' && ch != 'Q' && ch != 'M' && ch != 'E' && ch != 'C') return false;
  } else {
    if (ch != 0) return false;
  }

  if (ptr == end || *ptr != ',') return false;
  ptr++;

  uint32_t shares = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (shares > ((UINT32_MAX - (*ptr - '0')) / 10)) return false;
    shares = 10 * shares + (*ptr - '0');
    ptr++;
  }
  if ((t == 'S' || t == 'D') && shares > 0) return false;
  if (ptr == end) return false;
  output.shares = be32(shares);
  ptr++;
  if (ptr == end) return false;

  uint32_t price = 0;
  while (ptr < end && *ptr != ',') {
    if (!isdigit(*ptr)) return false;
    if (price > ((UINT32_MAX - (*ptr - '0')) / 10)) return false;
    price = price * 10 + (*ptr - '0');
    ptr++;
  }
  if ((t == 'S' || t == 'D' || t == 'E' || t == 'X') && price > 0) return false;
  if (ptr == end) return false;
  output.price = be32(price);
  ptr++;
  if (ptr == end) return false;

  if (t == 'A' || t == 'F') {
    for (int i = 0; i < 8; i++) {
      if (ptr + i == end || ptr[i] == ',') return false;
      output.stock[i] = ptr[i];
    }
    ptr += 8;
  } else {
    while (ptr < end && *ptr != ',') {
      ptr++;
    }
  }

  if (ptr == end || *ptr != ',') return false;
  ptr++;

  if (t == 'F') {
    for (int i = 0; i < 4; i++) {
      if (ptr + i == end || ptr[i] == ',') return false;
      output.attr[i] = ptr[i];
    }
    ptr += 4;
  } else {
    while (ptr < end && *ptr != ',') {
      ptr++;
    }
  }

  while (ptr < end && *ptr == ',') ptr++;

  return ptr == end;
}
