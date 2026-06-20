#include <cstdint>
#include <iostream>
#include <string>

#include "order-book.hpp"
#include "parser.hpp"

int main() {
  int32_t basePrice = 0;
  std::cout << "Input Base Price for the engine (be as close to the median as "
               "possible): ";
  std::cin >> basePrice;

  if (basePrice <= 0) {
    std::clog << "Base Price needs to be positive\n";
    return 1;
  }

  OrderBook book(basePrice);

  auto idx = 1;
  std::string line;
  while (std::getline(std::cin, line)) {
    Parser::ParsedOutput output;
    std::cout << idx++ << "\n";
    if (!Parser::parse(line, output)) {
      std::cerr << "Parse error in line: " << line << "\n";
      continue;
    }

    if (!book.consumeOrder(output)) {
      std::cerr << "Error in line: " << line << "\n";
    }
  }
}
