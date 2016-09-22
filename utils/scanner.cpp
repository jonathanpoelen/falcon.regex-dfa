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
  case s##_closure0:        \
  case s##_closure1:        \
  case s##_option

  unsigned n = 0;

  auto abeg = ctx.alter_list.begin();
  auto bbeg = ctx.bracket_list.begin();

  for (re::scanner_ctx::elem_t const & e : ctx.elems) {
    std::cout << std::setw(3) << n++ << " " << e.state;
    switch (e.state) {
      case re::regex_state::NB: break;
      case re::regex_state::bol: break;
      case re::regex_state::eol: break;
      case re::regex_state::terminate: break;
      case re::regex_state::alternation: break;
      case re::regex_state::start: break;
      CASE_WITH_QUANTI(re::regex_state::any): break;

      case re::regex_state::brace0:
        std::cout << "  {," << e.data.interval.m << "}";
        break;
      case re::regex_state::repetition:
        std::cout << "  {" << e.data.interval.n << "}";
        break;
      case re::regex_state::brace1:
        std::cout << "  {" << e.data.interval.n << ",}";
        break;
      case re::regex_state::interval:
        std::cout << "  {" << e.data.interval.n << "," << e.data.interval.m << "}";
        break;

      case re::regex_state::open:
      case re::regex_state::open_nocap:
      case re::regex_state::open_alternation:
      case re::regex_state::open_nocap_alternation:
        std::cout << "  idx_close: " << e.data.open.idx_close;
      case re::regex_state::start_alternation:
        std::cout << "  altern: ";
        for (auto && i : falcon::make_range(
          abeg + e.data.start.ibeg,
          abeg + e.data.start.iend
        )) {
          std::cout << i << ", ";
        }
        break;

      CASE_WITH_QUANTI(re::regex_state::close):
        std::cout << "  idx_open: " << e.data.close.idx_open;
        break;

      CASE_WITH_QUANTI(re::regex_state::single1):
      CASE_WITH_QUANTI(re::regex_state::escaped):
        std::cout << "  " << char(e.data.single.c);
        break;

      CASE_WITH_QUANTI(re::regex_state::bracket):
      CASE_WITH_QUANTI(re::regex_state::bracket_reverse):
        std::cout << "  [";
        for (auto && c : falcon::make_range(
          bbeg + e.data.bracket.ibeg,
          bbeg + e.data.bracket.iend
        )) {
          std::cout << char(c);
        }
        std::cout << "]";
        break;
    }
    std::cout << std::endl;
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
