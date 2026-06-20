#include "parser.hpp"

bool Parser::parse(std::string_view line, ParsedOutput &output) {
  auto ptr = line.data(), end = line.data() + line.size();
  if (ptr >= end || (*ptr != 'A' && *ptr != 'C' && *ptr != 'R')) return false;

  if (ptr + 2 >= end || *(ptr + 1) != ',') return false;
  output.action = *ptr;
  ptr += 2;

  auto id = 0;
  auto start = ptr;
  while (ptr < end && *ptr != ',') {
    if (__builtin_expect(*ptr >= '0' && *ptr <= '9', 1)) {
      id = id * 10 + (*ptr - '0');
      ptr++;
    } else {
      return false;
    }
  }
  if (ptr == start || ptr + 1 >= end || *ptr != ',') return false;
  output.orderID = id;
  ptr++;

  if (*ptr != 'A' && *ptr != 'B') return false;
  output.side = *ptr;
  ptr++;

  if (output.action == 'C') {
    return ptr == end;
  }
  if (ptr + 1 >= end || *ptr != ',') return false;
  ptr++;

  auto price = 0;
  start = ptr;
  while (ptr < end && *ptr != ',') {
    if (__builtin_expect(*ptr >= '0' && *ptr <= '9', 1)) {
      price = price * 10 + (*ptr - '0');
      ptr++;
    } else {
      return false;
    }
  }
  if (ptr == start || ptr + 1 >= end || *ptr != ',') return false;
  output.orderPrice = price;
  ptr++;

  auto qty = 0;
  start = ptr;
  while (ptr < end && *ptr != ',') {
    if (__builtin_expect(*ptr >= '0' && *ptr <= '9', 1)) {
      qty = qty * 10 + (*ptr - '0');
      ptr++;
    } else {
      return false;
    }
  }
  if (ptr == start) return false;
  output.orderQty = qty;

  return ptr == end;
}
