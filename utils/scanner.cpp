#include "falcon/regex_dfa/scanner/scanner.hpp"
#include <iostream>

namespace re = falcon::regex;

void print_scanner(re::scanner_ctx const & ctx)
{
  for (auto && e : ctx.elems) {
    std::cout << "{ " << static_cast<unsigned>(e.state) << "  " << e.idx_or_ch << " }\n";
  }
}

int main(int, char ** av) {
  if (av[1]) {
    char ** arr_str = av;
    while (*++arr_str) {
      std::cout << "pattern: \033[37;02m" << *arr_str << "\033[0m\n";
      print_scanner(re::scan(*arr_str));
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
        print_scanner(re::scan(s.c_str()));
      }
      catch (std::exception const & e) {
        std::cerr << e.what() << "\n";
      }
    }
  }
  return 0;
}
