#include "parser.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>

BufReader::BufReader(const uint8_t *data, uint32_t len)
    : cursor(data), end(data + len) {}

template <typename T>
T BufReader::readStruct() {
  T msg;
  std::memcpy(&msg, cursor, sizeof(T));
  cursor += sizeof(T);
  return msg;
}

ITCHMsg getMsg(BufReader &buf) {
  ITCHMsg msg = ITCHStructs::NullTp();
  switch (*buf.cursor) {
    case 'A':
      msg = buf.readStruct<ITCHStructs::AddOrder>();
      break;
    case 'F':
      msg = buf.readStruct<ITCHStructs::AddOrderMPID>();
      break;
    case 'D':
      msg = buf.readStruct<ITCHStructs::OrderDltMsg>();
      break;
    case 'X':
      msg = buf.readStruct<ITCHStructs::OrderCancelMsg>();
      break;
    case 'U':
      msg = buf.readStruct<ITCHStructs::OrderReplaceMsg>();
      break;
    case 'S':
      msg = buf.readStruct<ITCHStructs::SystemMsg>();
      break;
    default:
      break;
  }

  return msg;
}

UDPHdr readUDPHdr(BufReader &buf) {
  UDPHdr hdr;
  std::memcpy(&hdr, buf.cursor, 20);
  buf.cursor += 20;
  hdr.sessCnt = bs64(hdr.sessCnt);
  hdr.msgCnt = bs16(hdr.msgCnt);
  return hdr;
}

void handleMsgs(const char *path, OrderBook &book) {
  auto fd = open(path, O_RDONLY);
  if (fd < 0) {
    std::cerr << "ERROR: Could not open input data file: " << path << "\n";
    return;
  }

  struct stat buf;
  fstat(fd, &buf);

  auto data = static_cast<const uint8_t *>(
      mmap(nullptr, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
  close(fd);

  if (data == MAP_FAILED) {
    std::cerr << "Failed to map input file\n";
    return;
  }

  BufReader reader(data, buf.st_size);
  auto header = readUDPHdr(reader);

  while (header.msgCnt--) try {
      book.consumeMsg(getMsg(reader));
    } catch (const std::exception &e) {
      std::cerr << e.what() << "\n";
    }

  munmap((void *)data, buf.st_size);
}
