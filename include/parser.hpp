#pragma once

#include <cstdint>
#include <string_view>

namespace Parser {
struct ParsedOutput {
  char action;
  char side;
  uint32_t orderID;
  uint32_t orderPrice = -1;
  uint32_t orderQty = -1;
} __attribute__((packed));

bool parse(std::string_view line, ParsedOutput& output);
};  // namespace Parser
