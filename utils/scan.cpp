#include "falcon/regex_dfa/scan.hpp"
#include "falcon/regex_dfa/print_automaton.hpp"

#include <iostream>

namespace re = falcon::regex_dfa;

int main(int, char ** av) {
  if (av[1]) {
    char ** arr_str = av;
    while (*++arr_str) {
      std::cout << "pattern: \033[37;02m" << *arr_str << "\033[0m\n";
      re::print_automaton(re::scan(*arr_str));
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
      re::print_automaton(re::scan(s.c_str()));
    }
  }
  return 0;
}
