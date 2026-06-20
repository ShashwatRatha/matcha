#include <iostream>
#include <string>

#include "parser.hpp"

void prettyPrint(const Parser::ParsedOutput& output) {
  std::cout << "(\n"
            << "ID: " << output.orderID << "\n"
            << "Action: " << output.action << "\n"
            << "Side: " << output.side << "\n"
            << "Price: " << output.orderPrice << "\n"
            << "Qty: " << output.orderQty << "\n"
            << ")\n";
}

int main() {
  std::string line;
  while (std::getline(std::cin, line)) {
    if (line.empty()) continue;

    Parser::ParsedOutput output;
    if (!Parser::parse(line, output)) {
      std::cerr << "Parser failed at line: " << line << "\n";
    } else {
      prettyPrint(output);
    }
  }
}
