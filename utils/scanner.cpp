#include "falcon/regex_dfa/scanner/scanner.hpp"
#include "falcon/regex_dfa/scanner/scanner_io.hpp"
#include "falcon/regex_dfa/scanner/normalize.hpp"
#include "falcon/regex_dfa/scanner/match.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

namespace re = falcon::regex;

int main(int, char ** av) {
  if (av[1]) {
    char ** arr_str = av;
    while (*++arr_str) {
      std::cout << "pattern: \033[37;02m" << *arr_str << "\033[0m\n";
      auto scan_ctx = re::scan(*arr_str);
      std::cout << scan_ctx;
      while (*++arr_str) {
        std::cout << "match: " << re::match(scan_ctx, *arr_str) << " -- " << *arr_str << "\n";
      }
    }
  }
  else {
    std::string s;
    while (
      std::cout << "pattern: \033[37;02m",
      std::getline(std::cin, s),
      std::cout << "\033[0m\n",
      std::cin
    ) {
      try {
        auto scan_ctx = re::scan(s.c_str());
        std::cout << scan_ctx;
        while (
          std::cout << "str: \033[37;01m",
          std::getline(std::cin, s),
          std::cout << "\033[0m",
          std::cin
        ) {
          std::cout << "match: " << re::match(scan_ctx, s.c_str()) << "\n";
        }
      }
      catch (std::exception const & e) {
        std::cerr << e.what() << "\n";
      }
      std::cout << "\n";
      std::cin.clear();
    }
  }
  return 0;
}
