#include "print_automaton.hpp"
#include "redfa.hpp"

#include <iostream>
#include <iomanip>
#include <cassert>

namespace {
  constexpr char const * colors[]{
    "\033[31;02m",
    "\033[32;02m",
    "\033[33;02m",
    "\033[34;02m",
    "\033[35;02m",
    "\033[36;02m",
  };
  constexpr char const * reset_color = "\033[0m";

  template<class T>
  struct SetColor { T const & x; unsigned i; };

  template<class T>
  SetColor<T> setcolor(T const & x, unsigned i)
  {
    assert(i < sizeof(colors)/sizeof(colors[0]));
    return {x, i};
  }

  template<class T>
  std::ostream & operator<<(std::ostream & os, SetColor<T> x)
  { return os << colors[x.i] << x.x << reset_color; }
}

namespace falcon { namespace regex_dfa {

void print_automaton(const Transition & t)
{
  std::cout
    << colors[1]
    << "  [" << t.e.l << "-" << t.e.r << "]"
        " ['" << char(t.e.l) << "'-'" << char(t.e.r) << "']"
    << reset_color
    << " → " << t.next << "\n"
  ;
}

void print_automaton(const Transitions& ts)
{
  for (auto & t : ts) {
    std::cout
      << colors[1]
      << "  [" << t.e.l << "-" << t.e.r << "]"
          " ['" << char(t.e.l) << "'-'" << char(t.e.r) << "']"
      << reset_color
      << " → " << t.next << "\n"
    ;
  }
  std::cout << "\n";
}

void print_automaton(const Range& rng, int num)
{
  std::cout
      << std::setw(4) << num
      << colors[4]
      << (rng.states & Range::Final ? "@ " : "  ")
      << (rng.states & Range::Begin ? "^ " : "  ")
      << (rng.states & Range::End ? "$ " : "  ")
      << (rng.states & Range::Normal ? "= " : "  ")
      << (rng.states & Range::Invalid ? "x " : "  ")
      //<< (rng.states & Range::Epsilon ? "E " : "  ")
      << colors[3];
  for (auto & capstate : rng.capstates) {
    constexpr char const * c[]{
      "",
      "(",
      ")", "()",
      "+", "(+", "+)", "(+)",
      ".", ".(", ".)", ".()", ".+", ".(+", ".+)", ".(+)",
      "-", "-(", "-)", "-()", "-+", "-(+", "-+)", "-(+)",
        "-.", "-.(", "-.)", "-.()", "-.+", "-.(+", "-.+)", "-.(+)",
    };
    static_assert(sizeof(c)/sizeof(c[0]) == Capture::State::MAX, "");
    std::cout << c[capstate.e] << capstate.n << " ";
  }
  std::cout << reset_color << "\n";
  print_automaton(rng.transitions);
}

void print_automaton(const Ranges& rngs)
{
  for (auto & rng : rngs) {
    print_automaton(rng, int(&rng-&rngs[0]));
  }

  for (auto & caps : rngs.capture_table) {
    constexpr char c[]{'(',')'};
    std::cout << caps << " " << c[rngs.capture_table[caps] > caps] << "\n";
  }
}

} }