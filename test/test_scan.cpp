#include "falcon/regex_dfa/scan.hpp"
#include "falcon/regex_dfa/print_automaton.hpp"

#include <iostream>
// #include <cassert>

unsigned g_count = 0;

int main_result = 0;

namespace re = falcon::regex_dfa;

void test(
  char const * pattern
, re::Ranges const & rngs1
, re::Ranges const & rngs2
, unsigned line
) {
  if (rngs1 != rngs2) {
    std::cerr
      << ++g_count << "  line: " << line
      << "\npattern: \033[37;02m" << pattern
      << "\033[0m\n"
    ;
    re::print_automaton(rngs1);
    std::cerr << "value:\n";
    re::print_automaton(rngs2);
    std::cerr
      << " pattern: \033[37;02m" << pattern
      << "\033[0m\n"
    ;
    std::cerr << "----------\n";
    main_result = 1;
    //assert(false);
    //throw 1;
  }
}

#define TEST(pattern, value) test(pattern, re::scan(pattern), value, __LINE__)

using re::Range;
using re::Ranges;
using State = Range::State;
using re::Transition;
using re::Transitions;
using re::Event;

Transition t(Event e, std::size_t n) {
  return {e, n};
}

Transition t(re::char_int c, std::size_t n) {
  return {{c, c}, n};
}

Transition t(re::char_int c1, re::char_int c2, std::size_t n) {
  return {{c1, c2}, n};
}

Range r(State states) {
  return Range{states, {}, {}};
}

Range r(State states, Transitions const & ts) {
  return Range{states, {}, ts};
}

Range r(State states, Transitions && ts) {
  return Range{states, {}, std::move(ts)};
}

Range r() {
  return Range{Range::Normal, {}, {}};
}

Range r(Transitions const & ts) {
  return Range{Range::Normal, {}, ts};
}

Range r(Transitions && ts) {
  return Range{Range::Normal, {}, std::move(ts)};
}

template<class... Ts>
Ranges rs(Ts && ... r) {
  return Ranges{std::move(r)...};
}

int main()
{
  using ts = Transitions;
  Event const full {0, ~re::char_int{}};
  Range rf{Range::Final, {}, {}};

  auto const F = Range::Final;
  auto const B = Range::Begin;
  auto const E = Range::End;
  auto const N = Range::Normal;
  auto const BF = B | F;
  auto const BE = B | E;
  auto const BN = B | N;

  auto const ta1 = t('a', 1);
  auto const a1 = ts{ta1};

  TEST("", rs(rf));

  TEST("a", rs(r(a1), rf));
  TEST(".", rs(r(ts{t(full, 1)}), rf));

  TEST("a?", rs(r(F, a1), rf));
  TEST("a+", rs(r(a1), r(F, a1)));
  TEST("a*", rs(r(F, a1), r(F, a1)));

  auto const ta2 = t('a', 2);
  auto const ta3 = t('a', 3);
  auto const a2 = ts{ta2};
  auto const a3 = ts{ta3};
  auto const a1a2 = ts{ta1, ta2};
  auto const a1a2a3 = ts{ta1, ta2, ta3};
  auto const a2a3 = ts{ta2, ta3};

  TEST("a{1}", rs(r(a1), rf));
  TEST("a{2}", rs(r(a1), r(a2), rf));
  TEST("a{3}", rs(r(a1), r(a2), r(a3), rf));
  TEST("a{,1}", rs(r(F, a1), rf));
  TEST("a{,2}", rs(r(F, a1a2), r(F, a2), rf));
  TEST("a{,3}", rs(r(F, a1a2a3), r(F, a2a3), r(F, a3), rf));   
  TEST("a{1,}", rs(r(a1), r(F, a1)));
  TEST("a{2,}", rs(r(a1), r(a2), r(F, a2)));
  TEST("a{3,}", rs(r(a1), r(a2), r(a3), r(F, a3)));
  TEST("a{1,1}", rs(r(a1), rf));
  TEST("a{1,2}", rs(r(a1), r(F, a2), rf));
  TEST("a{1,3}", rs(r(a1), r(F, a2a3), r(F, a3), rf));
  TEST("a{2,2}", rs(r(a1), r(a2), rf));
  TEST("a{2,3}", rs(r(a1), r(a2), r(F, a3), rf));
  TEST("a{3,3}", rs(r(a1), r(a2), r(a3), rf));

  TEST("^$", rs(r(E)));
  TEST("^a?", rs(r(BF, a1), r(BF)));
  TEST("^a+", rs(r(a1), r(BF, a1)));
  TEST("^a*", rs(r(BF, a1), r(BF, a1)));

  TEST("a$", rs(r(a1), r(E)));
  TEST("a?$", rs(r(E, a1), r(E)));
  TEST("a+$", rs(r(a1), r(E, a1)));
  TEST("a*$", rs(r(E, a1), r(E, a1)));

  TEST("^a$", rs(r(a1), r(BE)));
  TEST("^a?$", rs(r(E, a1), r(BE)));
  TEST("^a+$", rs(r(a1), r(BE, a1)));
  TEST("^a*$", rs(r(E, a1), r(BE, a1)));


  auto const tb2 = t('b', 2);
  auto const b2 = ts{tb2};
  auto const a1b2 = ts{ta1, tb2};

  TEST("ab", rs(r(a1), r(b2), rf));

  TEST("a?b", rs(r(a1b2), r(b2), rf));
  TEST("a+b", rs(r(a1), r(a1b2), rf));
  TEST("a*b", rs(r(a1b2), r(a1b2), rf));
  
  auto const tb3 = t('b', 3);
  auto const tb4 = t('b', 4);
  auto const b3 = ts{tb3};
  auto const b4 = ts{tb4};
  auto const a1a2b3 = ts{ta1, ta2, tb3};
  auto const a1a2a3b4 = ts{ta1, ta2, ta3, tb4};
  auto const a2b3 = ts{ta2, tb3};
  auto const a2a3b4 = ts{ta2, ta3, tb4};
  auto const a3b4 = ts{ta3, tb4};

  TEST("a{1}b", rs(r(a1), r(b2), rf));
  TEST("a{2}b", rs(r(a1), r(a2), r(b3), rf));
  TEST("a{3}b", rs(r(a1), r(a2), r(a3), r(b4), rf));
  TEST("a{,1}b", rs(r(a1b2), r(b2), rf));
  TEST("a{,2}b", rs(r(a1a2b3), r(a2b3), r(b3), rf));
  TEST("a{,3}b", rs(r(a1a2a3b4), r(a2a3b4), r(a3b4), r(b4), rf));   
  TEST("a{1,}b", rs(r(a1), r(a1b2), rf));
  TEST("a{2,}b", rs(r(a1), r(a2), r(a2b3), rf));
  TEST("a{3,}b", rs(r(a1), r(a2), r(a3), r(a3b4), rf));
  TEST("a{1,1}b", rs(r(a1), r(b2), rf));
  TEST("a{1,2}b", rs(r(a1), r(a2b3), r(b3), rf));
  TEST("a{1,3}b", rs(r(a1), r(a2a3b4), r(a3b4), r(b4), rf));
  TEST("a{2,2}b", rs(r(a1), r(a2), r(b3), rf));
  TEST("a{2,3}b", rs(r(a1), r(a2), r(a3b4), r(b4), rf));
  TEST("a{3,3}b", rs(r(a1), r(a2), r(a3), r(b4), rf));

  TEST("^ab", rs(r(a1), r(BN, b2), rf));
  TEST("^a?b", rs(r(a1b2), r(BN, b2), r(BF)));
  TEST("^a+b", rs(r(a1), r(BN, a1b2), rf));
  TEST("^a*b", rs(r(a1b2), r(BN, a1b2), r(BF)));

  TEST("ab$", rs(r(a1), r(b2), r(E)));
  TEST("a?b$", rs(r(a1b2), r(b2), r(E)));
  TEST("a+b$", rs(r(a1), r(a1b2), r(E)));
  TEST("a*b$", rs(r(a1b2), r(a1b2), r(E)));

  TEST("^ab$", rs(r(a1), r(BN, b2), r(E)));
  TEST("^a?b$", rs(r(a1b2), r(BN, b2), r(BE)));
  TEST("^a+b$", rs(r(a1), r(BN, a1b2), r(E)));
  TEST("^a*b$", rs(r(a1b2), r(BN, a1b2), r(BE)));


  TEST("a?b?", rs(r(F, a1b2), r(F, b2), rf));
  TEST("a+b?", rs(r(a1), r(F, a1b2), rf));
  TEST("a*b?", rs(r(F, a1b2), r(F, a1b2), rf));

  TEST("a?b+", rs(r(a1b2), r(b2), r(F, b2)));
  TEST("a+b+", rs(r(a1), r(a1b2), r(F, b2)));
  TEST("a*b+", rs(r(a1b2), r(a1b2), r(F, b2)));

  TEST("a?b*", rs(r(F, a1b2), r(F, b2), r(F, b2)));
  TEST("a+b*", rs(r(a1), r(F, a1b2), r(F, b2)));
  TEST("a*b*", rs(r(F, a1b2), r(F, a1b2), r(F, b2)));


  auto const _ = t('-', 1);
  auto const a_b = t('a', 'b', 1);
  auto const b_c = t('b', 'c', 1);
  auto const c_d = t('c', 'd', 1);
  auto const a = t('a', 1);
  auto const b = t('b', 1);
  auto const c = t('c', 1);
  auto const d = t('d', 1);
  auto const ab1 = ts{a, b};

  TEST("[ab]", rs(r(ab1), rf));
  TEST("[-]", rs(r(ts{_}), rf));
  TEST("[a-b]", rs(r(ts{a_b}), rf));
  TEST("[-a-b]", rs(r(ts{_, a_b}), rf));
  TEST("[a-b-]", rs(r(ts{a_b, _}), rf));
  TEST("[-a-b-]", rs(r(ts{_, a_b, _}), rf));
  TEST("[ab-c]", rs(r(ts{a, b_c}), rf));
  TEST("[abc-d]", rs(r(ts{a, b, c_d}), rf));
  TEST("[ab-c-d]", rs(r(ts{a, b_c, _, d}), rf));
  TEST("[a-b-c]", rs(r(ts{a_b, _, c}), rf));
  TEST("[a-b-c-]", rs(r(ts{a_b, _, c, _}), rf));
  TEST("[a-b-c-d]", rs(r(ts{a_b, _, c_d}), rf));
  TEST("[a-b-c-d-]", rs(r(ts{a_b, _, c_d, _}), rf));
  TEST("[a-b-cd]", rs(r(ts{a_b, _, c, d}), rf));
  TEST("[a-b-cd-]", rs(r(ts{a_b, _, c, d, _}), rf));
  TEST("[a-bcd]", rs(r(ts{a_b, c, d}), rf));
  TEST("[a-bcd-]", rs(r(ts{a_b, c, d, _}), rf));

  auto const _a = t(0, 'a'-1, 1);
  auto const a_ = t('a'+1, ~re::char_int{}, 1);
  auto const b_ = t('b'+1, ~re::char_int{}, 1);
  auto const c_ = t('c'+1, ~re::char_int{}, 1);
  auto const d_ = t('d'+1, ~re::char_int{}, 1);
  auto const e_ = t('e'+1, ~re::char_int{}, 1);
  auto const f_ = t('f'+1, ~re::char_int{}, 1);

  TEST("[^a]", rs(r(ts{_a, a_}), rf));
  TEST("[^a-b]", rs(r(ts{_a, b_}), rf));
  TEST("[^ab]", rs(r(ts{_a, b_}), rf));
  TEST("[^ac]", rs(r(ts{_a, b, c_}), rf));
  TEST("[^acd]", rs(r(ts{_a, b, d_}), rf));
  TEST("[^ace]", rs(r(ts{_a, b, d, e_}), rf));
  TEST("[^a-cd-f]", rs(r(ts{_a, f_}), rf));
  TEST("[^a-ce-f]", rs(r(ts{_a, d, f_}), rf));

  TEST("[ab]?", rs(r(F, ab1), rf));
  TEST("[ab]+", rs(r(ab1), r(F, ab1)));
  TEST("[ab]*", rs(r(F, ab1), r(F, ab1)));


  return main_result;
}
