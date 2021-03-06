#include "falcon/regex_dfa/scan.hpp"
#include "falcon/regex_dfa/match.hpp"
#include "falcon/regex_dfa/print_automaton.hpp"

#include <iostream>

namespace re = falcon::regex_dfa;

int main(int, char ** av) {
  if (av[1]) {
    char ** arr_str = av;
    std::cout << "pattern: \033[37;02m" << *arr_str << "\033[0m\n";
    auto const rngs = re::scan(*arr_str);
    re::print_automaton(rngs);
    while (*++arr_str) {
      std::cout << "match: " << re::nfa_match(rngs, *arr_str) << " -- " << *arr_str << "\n";
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
        auto const rngs = re::scan(s.c_str());
        re::print_automaton(rngs);
        while (
          std::cout << "str: \033[37;01m", 
          std::getline(std::cin, s), 
          std::cout << "\033[0m", 
          std::cin
        ) {
          std::cout << "match: " << re::nfa_match(rngs, s.c_str()) << "\n";
        }
        std::cin.clear();
        std::cout << "\n";
      }
      catch (std::exception const & e) {
        std::cerr << e.what() << "\n";
      }
    }
  }
  return 0;
}
