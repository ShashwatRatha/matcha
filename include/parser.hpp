#pragma once

#include <cstdint>

#include "itch-structs.hpp"
#include "order-book.hpp"

struct BufReader {
  BufReader(const uint8_t* data, uint32_t len);
  BufReader(const BufReader&) = delete;
  BufReader(BufReader&&) = delete;
  BufReader& operator=(const BufReader&) = delete;
  BufReader& operator=(BufReader&&) = delete;

  const uint8_t* end;
  const uint8_t* cursor;

  template <typename T>
  T readStruct();
};

struct UDPHdr {
  char sessName[10];
  uint64_t sessCnt;
  uint16_t msgCnt;
} __attribute__((packed));

UDPHdr readUDPHdr(BufReader& buf);
ITCHMsg getMsg(BufReader& buf);
void handleMsgs(const char* path, OrderBook& book);
