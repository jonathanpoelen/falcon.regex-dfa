#include "falcon/regex_dfa/scanner/scanner.hpp"
#include "falcon/regex_dfa/range_iterator.hpp"
#include <iostream>
#include <iomanip>
#include <cassert>

namespace re = falcon::regex;

void print_scanner(re::scanner_ctx const & ctx)
{
#define CASE_WITH_QUANTI(s) \
  case s:                   \
  case s##_closure0:         \
  case s##_closure1:         \
  case s##_option:           \
  case s##_brace0:           \
  case s##_repetition:       \
  case s##_brace1:           \
  case s##_interval

  auto const pbeg = ctx.params.begin();
  auto pit = pbeg;

  auto print_quanti = [&pit](re::regex_state st){
    unsigned u = static_cast<unsigned>(st);
    u -= static_cast<unsigned>(re::regex_state::single1);
    u %= 8;
    switch (u) {
      case 4:
        std::cout << "  {," << *pit << "}";
        ++pit;
        break;
      case 5:
        std::cout << "  {" << *pit << "}";
        ++pit;
        break;
      case 6:
        std::cout << "  {" << *pit << ",}";
        ++pit;
        break;
      case 7:
        std::cout << "  {" << *pit << ", " << *(pit + 1) << "}";
        pit += 2;
        break;
    }
  };

  unsigned n = 0;

  for (auto && e : ctx.elems) {
    std::cout << std::setw(3) << n++ << " " << e;
    switch (e) {
      case re::regex_state::NB: break;
      case re::regex_state::start: break;
      case re::regex_state::bol: break;
      case re::regex_state::eol: break;
      case re::regex_state::open: break;
      case re::regex_state::open_nocap: break;
      case re::regex_state::terminate: break;
      case re::regex_state::alternation: break;

      case re::regex_state::open_alternation:
      case re::regex_state::open_nocap_alternation:
        std::cout << "  idx_close: " << *pit;
        ++pit;
      case re::regex_state::start_alternation:
        std::cout << "  altern: ";
        assert(pbeg + *pit < pbeg + *(pit + 1u));
        for (auto && i : falcon::make_range(pbeg + *pit, pbeg + *(pit + 1u))) {
          std::cout << i << ", ";
        }
        pit += 2;
        break;

      CASE_WITH_QUANTI(re::regex_state::single1):
      CASE_WITH_QUANTI(re::regex_state::escaped):
        std::cout << "  " << char(*pit);
        ++pit;
        print_quanti(e);
        break;

      CASE_WITH_QUANTI(re::regex_state::any):
        print_quanti(e);
        break;

      CASE_WITH_QUANTI(re::regex_state::bracket):
      CASE_WITH_QUANTI(re::regex_state::bracket_reverse):
        std::cout << "  [";
        assert(pit <= pbeg + *pit);
        for (auto && c : falcon::make_range(pit, pbeg + *pit)) {
          std::cout << char(c);
        }
        std::cout << "]";
        pit = pbeg + *pit;
        print_quanti(e);
        break;

      CASE_WITH_QUANTI(re::regex_state::close):
        std::cout << "  idx_open: " << *pit;
        pit = pbeg + pbeg[*(pit + 1u) + 2];
        print_quanti(e);
        break;
    }
    std::cout << std::endl;
  }
  std::cout << "\n";
  for (auto && e : ctx.params) {
    std::cout << e << ", ";
  }
  std::cout << "\n";
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
