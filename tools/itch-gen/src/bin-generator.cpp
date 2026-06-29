#include "bin-generator.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "parser.hpp"

template <typename Tp>
static inline void populateHdr(const Parser::ParsedOutput& src, Tp& dst) {
  dst.msgType = src.msgType;
  dst.stockLocate = src.stockLocate;
  dst.trackNum = src.trackNum;
  std::memcpy(dst.timestamp, src.timestamp, 6);
}

template <typename T, typename Fn>
static void serialise(const Parser::ParsedOutput& p, Fn f) {
  T msg;
  populateHdr(p, msg);
  f(msg);

  uint16_t size = __builtin_bswap16(static_cast<uint16_t>(sizeof(T)));

  std::cout.write(reinterpret_cast<const char*>(&size), 2);
  std::cout.write(reinterpret_cast<const char*>(&msg), sizeof(T));
}

static void generateData(const Parser::ParsedOutput& p) {
  switch (p.msgType) {
    case 'A': {
      serialise<MsgStructs::AddOrderWithoutMPID>(p, [&](auto& msg) {
        msg.orderRefNum = p.orderRefNum;
        msg.bsInd = p.bsEvCd;
        msg.shares = p.shares;
        std::memcpy(msg.stock, p.stock, 8);
        msg.price = p.price;
      });
    } break;
    case 'F': {
      serialise<MsgStructs::AddOrderWithMPID>(p, [&](auto& msg) {
        msg.orderRefNum = p.orderRefNum;
        msg.bsInd = p.bsEvCd;
        msg.shares = p.shares;
        std::memcpy(msg.stock, p.stock, 8);
        msg.price = p.price;
        std::memcpy(msg.attr, p.attr, 4);
      });
    } break;
    case 'S': {
      serialise<MsgStructs::SystemMsg>(
          p, [&](auto& msg) { msg.eventCode = p.bsEvCd; });
    } break;
    case 'D': {
      serialise<MsgStructs::OrderDltMsg>(
          p, [&](auto& msg) { msg.orderRefNum = p.orderRefNum; });
    } break;
    case 'X': {
      serialise<MsgStructs::OrderCancelMsg>(p, [&](auto& msg) {
        msg.orderRefNum = p.orderRefNum;
        msg.cancelShares = p.shares;
      });
    } break;
    case 'U': {
      serialise<MsgStructs::OrderReplaceMsg>(p, [&](auto& msg) {
        msg.orgORN = p.orderRefNum;
        msg.newORN = p.newORN;
        msg.newShares = p.shares;
        msg.newPrice = p.price;
      });
    } break;
    default:
      break;
  }
}

std::string countPrompt =
    "Enter number of messages in file (Max: 65535) \n(Enter -1 if 1st line of "
    "file is the message count): ";
std::string finishMsg = "Finished writing binary data\n";

void generateBinFile() {
  std::fstream tty("/dev/tty");
  if (!tty.is_open()) {
    std::cerr << "Unable to open controlling terminal for comms\n";
    return;
  }

  struct MoldUDP64Hdr {
    char sToken[10];
    uint64_t sCounter;
    uint16_t msgCount;
  } __attribute__((packed)) hdr;

  tty << countPrompt;

  int32_t numMsg = 0;
  tty >> numMsg;

  if (numMsg < 0) {
    std::string num;
    std::cin >> num;
    numMsg = std::stoi(num);
  }

  if (numMsg > UINT16_MAX) {
    std::cerr << "Entered message count exceeds 65535, cant accept\n";
    tty.close();
    return;
  }

  std::memcpy(hdr.sToken, "SESS123456", 10);
  hdr.sCounter = __builtin_bswap64(1);
  hdr.msgCount = __builtin_bswap16(static_cast<uint16_t>(numMsg));

  std::cout.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

  std::string line;
  while (std::getline(std::cin, line)) {
    Parser::ParsedOutput p;
    if (!Parser::parse(line, p)) {
      std::cerr << "Parse Error occurred in line: " << line << "\n";
      continue;
    }

    generateData(p);
  }

  tty << finishMsg;
  tty.close();
}
