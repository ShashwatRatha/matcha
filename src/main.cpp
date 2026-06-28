#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <iostream>

#include "order-book.hpp"
#include "parser.hpp"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <path to binary file>\n";
    return 2;
  }
  uint32_t basePrice;
  std::cout << "Input BasePrice for the stock (be as close to median as "
               "possible): \n";
  std::cin >> basePrice;
  OrderBook book(basePrice);

  int tradeFile = open("trade.log", O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (tradeFile < 0) std::cerr << "Couldnt log trades in trade.log file\n";
  int errorFile = open("error.log", O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (errorFile < 0) std::cerr << "Couldnt log errors in error.log file\n";

  dup2(tradeFile, STDOUT_FILENO);
  dup2(errorFile, STDERR_FILENO);

  handleMsgs(argv[1], book);

  close(tradeFile);
  close(errorFile);
}
