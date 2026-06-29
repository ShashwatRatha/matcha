#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>

// Endianness Helpers (Big-Endian network to Little-Endian host conversion)
inline uint16_t le16(uint16_t val) { return __builtin_bswap16(val); }
inline uint32_t le32(uint32_t val) { return __builtin_bswap32(val); }
inline uint64_t le64(uint64_t val) { return __builtin_bswap64(val); }

inline uint64_t parse48BitTimestamp(const uint8_t* ts) {
  uint64_t result = 0;
  for (int i = 0; i < 6; ++i) {
    result = (result << 8) | ts[i];
  }
  return result;
}

// Protocol Structs matching your layout specifications
struct MoldUDP64Hdr {
  char sToken[10];
  uint64_t sCounter;
  uint16_t msgCount;
} __attribute__((packed));

namespace ITCH50 {
struct SystemMsg {
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint8_t eventCode;
} __attribute__((packed));

struct AddOrderWithoutMPID {
  uint8_t msgType;
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
  uint8_t msgType;
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

struct OrderExecutedMsg {
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint32_t execShares;
  uint64_t matchNum;
} __attribute__((packed));

struct OrderExecutedWithPriceMsg {
  uint8_t msgType;
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
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
  uint32_t cancelShares;
} __attribute__((packed));

struct OrderDeleteMsg {
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t orderRefNum;
} __attribute__((packed));

struct OrderReplaceMsg {
  uint8_t msgType;
  uint16_t stockLocate;
  uint16_t trackNum;
  uint8_t timestamp[6];
  uint64_t origORN;
  uint64_t newORN;
  uint32_t newShares;
  uint32_t newPrice;
} __attribute__((packed));
}  // namespace ITCH50

void printStock(const uint8_t* stock) {
  std::cout << "Stock: ";
  for (int i = 0; i < 8; ++i) {
    if (stock[i] != ' ' && stock[i] != 0) std::cout << (char)stock[i];
  }
}

void printMPID(const uint8_t* attr) {
  std::cout << "MPID: ";
  for (int i = 0; i < 4; ++i) {
    if (attr[i] != ' ' && attr[i] != 0) std::cout << (char)attr[i];
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_output_file.bin>\n";
    return 1;
  }

  std::ifstream file(argv[1], std::ios::binary);
  if (!file.is_open()) {
    std::cerr << "Error opening file: " << argv[1] << "\n";
    return 1;
  }

  // 1. Unpack MoldUDP64 Session Header
  MoldUDP64Hdr hdr;
  if (!file.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) {
    std::cerr << "Failed to read MoldUDP64 Header\n";
    return 1;
  }

  std::string token(hdr.sToken, 10);
  std::cout << "==================================================\n";
  std::cout << "MOLDUDP64 HEADER STREAM RECORD\n";
  std::cout << "==================================================\n";
  std::cout << "Session Token : " << token << "\n";
  std::cout << "Seq Counter   : " << le64(hdr.sCounter) << "\n";
  std::cout << "Msg Count     : " << le16(hdr.msgCount) << "\n";
  std::cout << "==================================================\n\n";

  uint32_t processedCount = 0;

  // 2. Loop through individual prefixed ITCH message payloads
  while (file.peek() != EOF) {
    uint16_t blockLength = 0;
    if (!file.read(reinterpret_cast<char*>(&blockLength), 2)) break;
    blockLength = le16(blockLength);

    // Safely fetch payload into buffer
    char buffer[64];
    if (!file.read(buffer, blockLength)) {
      std::cerr << "Corrupted payload packet boundary encountered.\n";
      break;
    }

    uint8_t msgType = static_cast<uint8_t>(buffer[0]);
    processedCount++;

    std::cout << "[" << processedCount << "] Type '" << (char)msgType
              << "' (Payload size: " << blockLength << " bytes)\n";

    switch (msgType) {
      case 'S': {
        auto* m = reinterpret_cast<ITCH50::SystemMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp)
                  << " | Event: " << m->eventCode << "\n";
        break;
      }
      case 'A': {
        auto* m = reinterpret_cast<ITCH50::AddOrderWithoutMPID*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n    ";
        printStock(m->stock);
        std::cout << " | Side: " << m->bsInd << " | Shares: " << le32(m->shares)
                  << " | Price: " << le32(m->price)
                  << " | OrderRefNum: " << le64(m->orderRefNum) << "\n";
        break;
      }
      case 'F': {
        auto* m = reinterpret_cast<ITCH50::AddOrderWithMPID*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n    ";
        printStock(m->stock);
        std::cout << " | Side: " << m->bsInd << " | Shares: " << le32(m->shares)
                  << " | Price: " << le32(m->price)
                  << " | OrderRefNum: " << le64(m->orderRefNum) << " | ";
        printMPID(m->attr);
        std::cout << "\n";
        break;
      }
      case 'E': {
        auto* m = reinterpret_cast<ITCH50::OrderExecutedMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n"
                  << "    OrderRefNum: " << le64(m->orderRefNum)
                  << " | ExecShares: " << le32(m->execShares)
                  << " | MatchNum: " << le64(m->matchNum) << "\n";
        break;
      }
      case 'C': {
        auto* m = reinterpret_cast<ITCH50::OrderExecutedWithPriceMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n"
                  << "    OrderRefNum: " << le64(m->orderRefNum)
                  << " | ExecShares: " << le32(m->execShares)
                  << " | MatchNum: " << le64(m->matchNum)
                  << " | Printable: " << m->printable
                  << " | ExecPrice: " << le32(m->execPrice) << "\n";
        break;
      }
      case 'X': {
        auto* m = reinterpret_cast<ITCH50::OrderCancelMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n"
                  << "    OrderRefNum: " << le64(m->orderRefNum)
                  << " | CancelShares: " << le32(m->cancelShares) << "\n";
        break;
      }
      case 'D': {
        auto* m = reinterpret_cast<ITCH50::OrderDeleteMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp)
                  << " | OrderRefNum: " << le64(m->orderRefNum) << "\n";
        break;
      }
      case 'U': {
        auto* m = reinterpret_cast<ITCH50::OrderReplaceMsg*>(buffer);
        std::cout << "    Locate: " << le16(m->stockLocate)
                  << " | Track: " << le16(m->trackNum)
                  << " | TS: " << parse48BitTimestamp(m->timestamp) << "\n"
                  << "    OrigOrderRefNum: " << le64(m->origORN)
                  << " | NewOrderRefNum: " << le64(m->newORN)
                  << " | NewShares: " << le32(m->newShares)
                  << " | NewPrice: " << le32(m->newPrice) << "\n";
        break;
      }
      default:
        std::cout << "    Unknown ITCH Message type signature.\n";
        break;
    }
    std::cout << "--------------------------------------------------\n";
  }

  file.close();
  return 0;
}
