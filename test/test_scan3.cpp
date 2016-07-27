#include "falcon/regex_dfa/scan.hpp"
#include "falcon/regex_dfa/print_automaton.hpp"

#include <iostream>
#include <algorithm>
// #include <cassert>

unsigned count_test_failure = 0;
unsigned count_test = 0;

namespace re = falcon::regex_dfa;

void test(
  char const * pattern
, re::Ranges const & expected
, unsigned line
) {
  re::Ranges rngs;
  try {
    rngs = re::scan(pattern);
  }
  catch (std::exception const & e) {
    std::cerr << e.what() << "\n";
    if (rngs.size() == expected.size()) {
      ++count_test_failure;
    }
  }
  if (![&]{
    if (rngs.size() != expected.size()) {
      std::cerr << "# different size\n";
      return false;
    }
    auto p1 = rngs.begin();
    auto neq = [&p1](re::Range const & r) {
      bool const ret = r.states ? *p1 != r : false;
      ++p1;
      return ret;
    };
    auto p2 = std::find_if(expected.begin(), expected.end(), neq);
    if (p2 != expected.end()) {
      std::cerr << "# different range (n. " << (p2-expected.begin()) << ")\n";
      return false;
    }
    return true;
  }()) {
    std::cerr
      << ++count_test_failure << "  line: " << line
      << "\npattern: \033[37;02m" << pattern
      << "\033[0m\n"
    ;
    re::print_automaton(rngs);
    std::cerr << "expected:\n";
    re::print_automaton(expected);
    std::cerr
      << " pattern: \033[37;02m" << pattern
      << "\033[0m\n"
    ;
    std::cerr << "----------\n";
    //assert(false);
    //throw 1;
  }
  ++count_test;
}

#define TEST(pattern, value) test(pattern, value, __LINE__)

using re::Range;
using re::Ranges;
using State = Range::State;
using re::Transition;
using re::Transitions;
using re::Event;

Transition t(Event e, unsigned n) {
  return {e, n};
}

Transition t(re::char_int c, unsigned n) {
  return {{c, c}, n};
}

Transition t(re::char_int c1, re::char_int c2, unsigned n) {
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
  Range none{{}, {}, {}};

  auto const F = Range::Final;
  auto const N = Range::Normal;
  auto const B = Range::Bol;
  auto const E = Range::Eol;
  auto const C = Range::Empty;
  auto const BF = B | F;
  auto const BE = B | E;
  auto const NF = N | F;
  auto const NE = N | E;
  //auto const NBF = N | BF;
  //auto const NBE = N | BE;

  auto const rf = r(F);
  auto const ru = r(Range::None);

  auto const ta1 = t('a', 1);
  auto const ta2 = t('a', 2);
  auto const a2 = ts{ta2};
  auto const a1 = ts{ta1};

  TEST("", rs(ru, rf));

  TEST("a", rs(ru, r(a2), rf));
  TEST(".", rs(ru, r(ts{t(full, 2)}), rf));

  TEST("a?", rs(ru, r(NF, a2), rf));
  TEST("a+", rs(ru, r(a2), r(NF, a2)));
  TEST("a*", rs(ru, r(NF, a2), r(NF, a2)));

  TEST("^$", rs(r(C), ru));

  TEST("^a", rs(r(B, a2), ru, rf));
  TEST("^a?", rs(r(BF, a2), ru, rf));
  TEST("^a+", rs(r(B, a2), ru, r(NF, a2)));
  TEST("^a*", rs(r(BF, a2), ru, r(NF, a2)));

  TEST("a$", rs(ru, r(a2), r(E)));
  TEST("a?$", rs(ru, r(NE, a2), r(E)));
  TEST("a+$", rs(ru, r(a2), r(NE, a2)));
  TEST("a*$", rs(ru, r(NE, a2), r(NE, a2)));

  TEST("^a$", rs(r(B, a2), ru, r(E)));
  TEST("^a?$", rs(r(BE, a2), ru, r(E)));
  TEST("^a+$", rs(r(B, a2), ru, r(NE, a2)));
  TEST("^a*$", rs(r(BE, a2), ru, r(NE, a2)));

  auto const tb3 = t('b', 3);
  auto const a2b3 = ts{ta2, tb3};
  auto const b3 = ts{tb3};

  TEST("ab", rs(ru, r(a2), r(b3), rf));

  TEST("a?b", rs(ru, r(a2b3), r(b3), rf));
  TEST("a+b", rs(ru, r(a2), r(a2b3), rf));
  TEST("a*b", rs(ru, r(a2b3), r(a2b3), rf));

  TEST("^ab", rs(r(B, a2), ru, r(b3), rf));
  TEST("^a?b", rs(r(B, a2b3), ru, r(b3), rf));
  TEST("^a+b", rs(r(B, a2), ru, r(a2b3), rf));
  TEST("^a*b", rs(r(B, a2b3), ru, r(a2b3), rf));

  TEST("ab$", rs(ru, r(a2), r(b3), r(E)));
  TEST("a?b$", rs(ru, r(a2b3), r(b3), r(E)));
  TEST("a+b$", rs(ru, r(a2), r(a2b3), r(E)));
  TEST("a*b$", rs(ru, r(a2b3), r(a2b3), r(E)));

  TEST("^ab$", rs(r(B, a2), ru, r(b3), r(E)));
  TEST("^a?b$", rs(r(B, a2b3), ru, r(b3), r(E)));
  TEST("^a+b$", rs(r(B, a2), ru, r(a2b3), r(E)));
  TEST("^a*b$", rs(r(B, a2b3), ru, r(a2b3), r(E)));

  TEST("a?b?", rs(ru, r(NF, a2b3), r(NF, b3), rf));
  TEST("a+b?", rs(ru, r(a2), r(NF, a2b3), rf));
  TEST("a*b?", rs(ru, r(NF, a2b3), r(NF, a2b3), rf));

  TEST("a?b+", rs(ru, r(a2b3), r(b3), r(NF, b3)));
  TEST("a+b+", rs(ru, r(a2), r(a2b3), r(NF, b3)));
  TEST("a*b+", rs(ru, r(a2b3), r(a2b3), r(NF, b3)));

  auto const tb2 = t('b', 2);
  auto const b2 = ts{tb2};
  auto const a2b2 = ts{ta2, tb2};

  TEST("a?b*", rs(ru, r(NF, a2b2), r(NF, b2)));
  TEST("a+b*", rs(ru, r(a2), r(NF, a2b3), r(NF, b3)));
  TEST("a*b*", rs(ru, r(NF, a2b3), r(NF, a2b3), r(NF, b3)));

  auto const tc4 = t('c', 4);
  auto const c4 = ts{tc4};

  TEST("a?bc", rs(ru, r(a2b3), r(b3), r(c4), rf));
  TEST("a+bc", rs(ru, r(a2), r(a2b3), r(c4), rf));
  TEST("a*bc", rs(ru, r(a2b3), r(a2b3), r(c4), rf));

  TEST("^a?bc", rs(r(B, a2b3), ru, r(b3), r(c4), rf));
  TEST("^a+bc", rs(r(B, a2), ru, r(a2b3), r(c4), rf));
  TEST("^a*bc", rs(r(B, a2b3), ru, r(a2b3), r(c4), rf));

  TEST("a?bc$", rs(ru, r(a2b3), r(b3), r(c4), r(E)));
  TEST("a+bc$", rs(ru, r(a2), r(a2b3), r(c4), r(E)));
  TEST("a*bc$", rs(ru, r(a2b3), r(a2b3), r(c4), r(E)));

  TEST("^abc$", rs(r(B, a2), ru, r(b3), r(c4), r(E)));
  TEST("^a?bc$", rs(r(B, a2b3), ru, r(b3), r(c4), r(E)));
  TEST("^a+bc$", rs(r(B, a2), ru, r(a2b3), r(c4), r(E)));
  TEST("^a*bc$", rs(r(B, a2b3), ru, r(a2b3), r(c4), r(E)));

  auto const a2b3c4 = ts{ta2, tb3, tc4};
  auto const b3c4 = ts{tb3, tc4};

  TEST("a?b?c", rs(ru, r(a2b3c4), r(b3c4), r(c4), rf));
  TEST("a+b?c", rs(ru, r(a2), r(a2b3c4), r(c4), rf));
  TEST("a*b?c", rs(ru, r(a2b3c4), r(a2b3c4), r(c4), rf));

  TEST("a?b+c", rs(ru, r(a2b3), r(b3), r(b3c4), rf));
  TEST("a+b+c", rs(ru, r(a2), r(a2b3), r(b3c4), rf));
  TEST("a*b+c", rs(ru, r(a2b3), r(a2b3), r(b3c4), rf));

  auto const a2b2c4 = ts{ta2, tb2, tc4};
  auto const b3b2c4 = ts{tb3, tb2, tc4};

  TEST("a?b*c", rs(ru, r(a2b2c4), r(b3b2c4), r(c4), rf));
  TEST("a+b*c", rs(ru, r(a2), r(a2b3c4), r(b3c4), rf));
  TEST("a*b*c", rs(ru, r(a2b3c4), r(a2b3c4), r(b3c4), rf));

  auto const td5 = t('d', 5);
  auto const d5 = ts{td5};

  TEST("a?b?cd", rs(ru, r(a2b3c4), r(b3c4), r(c4), r(d5), rf));
  TEST("a+b?cd", rs(ru, r(a2), r(a2b3c4), r(c4), r(d5), rf));
  TEST("a*b?cd", rs(ru, r(a2b3c4), r(a2b3c4), r(c4), r(d5), rf));

  TEST("a?b+cd", rs(ru, r(a2b3), r(b3), r(b3c4), r(d5), rf));
  TEST("a+b+cd", rs(ru, r(a2), r(a2b3), r(b3c4), r(d5), rf));
  TEST("a*b+cd", rs(ru, r(a2b3), r(a2b3), r(b3c4), r(d5), rf));

  TEST("a?b*cd", rs(ru, r(a2b2c4), r(b3b2c4), r(c4), r(d5), rf));
  TEST("a+b*cd", rs(ru, r(a2), r(a2b3c4), r(b3c4), r(d5), rf));
  TEST("a*b*cd", rs(ru, r(a2b3c4), r(a2b3c4), r(b3c4), r(d5), rf));

//   auto const ta2 = t('a', 2);
//   auto const ta3 = t('a', 3);
//   auto const a1a2 = ts{ta1, ta2};
//   auto const a1a2a3 = ts{ta1, ta2, ta3};
//   auto const a2 = ts{ta2};
//   auto const a2a3 = ts{ta2, ta3};
//   auto const a3 = ts{ta3};
//
//   TEST("a{1}", rs(r(a1), rf));
//   TEST("a{2}", rs(r(a1), r(a2), rf));
//   TEST("a{3}", rs(r(a1), r(a2), r(a3), rf));
//   TEST("a{,1}", rs(r(F, a1), rf));
//   TEST("a{,2}", rs(r(F, a1a2), r(F, a2), rf));
//   TEST("a{,3}", rs(r(F, a1a2a3), r(F, a2a3), r(F, a3), rf));
//   TEST("a{1,}", rs(r(a1), r(F, a1)));
//   TEST("a{2,}", rs(r(a1), r(a2), r(F, a2)));
//   TEST("a{3,}", rs(r(a1), r(a2), r(a3), r(F, a3)));
//   TEST("a{1,1}", rs(r(a1), rf));
//   TEST("a{1,2}", rs(r(a1), r(F, a2), rf));
//   TEST("a{1,3}", rs(r(a1), r(F, a2a3), r(F, a3), rf));
//   TEST("a{2,2}", rs(r(a1), r(a2), rf));
//   TEST("a{2,3}", rs(r(a1), r(a2), r(F, a3), rf));
//   TEST("a{3,3}", rs(r(a1), r(a2), r(a3), rf));

//   auto const tb3 = t('b', 3);
//   auto const tb4 = t('b', 4);
//   auto const a1a2a3b4 = ts{ta1, ta2, ta3, tb4};
//   auto const a1a2b3 = ts{ta1, ta2, tb3};
//   auto const a2a3b4 = ts{ta2, ta3, tb4};
//   auto const a2b3 = ts{ta2, tb3};
//   auto const a3b4 = ts{ta3, tb4};
//   auto const b3 = ts{tb3};
//   auto const b4 = ts{tb4};
//
//   TEST("a{1}b", rs(r(a1), r(b2), rf));
//   TEST("a{2}b", rs(r(a1), r(a2), r(b3), rf));
//   TEST("a{3}b", rs(r(a1), r(a2), r(a3), r(b4), rf));
//   TEST("a{,1}b", rs(r(a1b2), r(b2), rf));
//   TEST("a{,2}b", rs(r(a1a2b3), r(a2b3), r(b3), rf));
//   TEST("a{,3}b", rs(r(a1a2a3b4), r(a2a3b4), r(a3b4), r(b4), rf));
//   TEST("a{1,}b", rs(r(a1), r(a1b2), rf));
//   TEST("a{2,}b", rs(r(a1), r(a2), r(a2b3), rf));
//   TEST("a{3,}b", rs(r(a1), r(a2), r(a3), r(a3b4), rf));
//   TEST("a{1,1}b", rs(r(a1), r(b2), rf));
//   TEST("a{1,2}b", rs(r(a1), r(a2b3), r(b3), rf));
//   TEST("a{1,3}b", rs(r(a1), r(a2a3b4), r(a3b4), r(b4), rf));
//   TEST("a{2,2}b", rs(r(a1), r(a2), r(b3), rf));
//   TEST("a{2,3}b", rs(r(a1), r(a2), r(a3b4), r(b4), rf));
//   TEST("a{3,3}b", rs(r(a1), r(a2), r(a3), r(b4), rf));

//   auto const B, a1B, a1b2 = ts{tB(ta1), tB(ta1), tB(tb2)};
//   auto const a1a1 = ts{ta1, ta1};
//   auto const a1a1b2 = ts{ta1, ta1, tb2};
//   auto const a1a1b2b2B = ts{ta1, ta1, tB(tb2), tB(tb2)};
//   auto const a1b2b2B = ts{ta1, tB(tb2), tB(tb2)};
//   auto const b2b2 = ts{tb2, tb2};
//
//   TEST("^(?!ab){1,}", rs(r(B, a1), r(b2), r(F, a1)));
//   TEST("^(?!a*){1,}", rs(r(BF, B, a1B, a1), r(F, a1a1)));
//   TEST("^(?!a*){1,}b", rs(r(B, a1B, a1b2), r(a1a1b2), rf));
//   TEST("a*^(?!b*){1,}", rs(r(a1a1b2b2B), r(a1b2b2B), r(F, b2b2)));
//
//   auto const _ = t('-', 1);
//   auto const a_b = t('a', 'b', 1);
//   auto const b_c = t('b', 'c', 1);
//   auto const c_d = t('c', 'd', 1);
//   auto const a = t('a', 1);
//   auto const b = t('b', 1);
//   auto const c = t('c', 1);
//   auto const d = t('d', 1);
//   auto const ab1 = ts{a, b};
//
//   TEST("[ab]", rs(r(ab1), rf));
//   TEST("[-]", rs(r(ts{_}), rf));
//   TEST("[a-b]", rs(r(ts{a_b}), rf));
//   TEST("[-a-b]", rs(r(ts{_, a_b}), rf));
//   TEST("[a-b-]", rs(r(ts{a_b, _}), rf));
//   TEST("[-a-b-]", rs(r(ts{_, a_b, _}), rf));
//   TEST("[ab-c]", rs(r(ts{a, b_c}), rf));
//   TEST("[abc-d]", rs(r(ts{a, b, c_d}), rf));
//   TEST("[ab-c-d]", rs(r(ts{a, b_c, _, d}), rf));
//   TEST("[a-b-c]", rs(r(ts{a_b, _, c}), rf));
//   TEST("[a-b-c-]", rs(r(ts{a_b, _, c, _}), rf));
//   TEST("[a-b-c-d]", rs(r(ts{a_b, _, c_d}), rf));
//   TEST("[a-b-c-d-]", rs(r(ts{a_b, _, c_d, _}), rf));
//   TEST("[a-b-cd]", rs(r(ts{a_b, _, c, d}), rf));
//   TEST("[a-b-cd-]", rs(r(ts{a_b, _, c, d, _}), rf));
//   TEST("[a-bcd]", rs(r(ts{a_b, c, d}), rf));
//   TEST("[a-bcd-]", rs(r(ts{a_b, c, d, _}), rf));
//
//   auto const _a = t('\0', 'a'-1, 1);
//   auto const a_ = t('a'+1, ~re::char_int{}, 1);
//   auto const b_ = t('b'+1, ~re::char_int{}, 1);
//   auto const c_ = t('c'+1, ~re::char_int{}, 1);
//   auto const d_ = t('d'+1, ~re::char_int{}, 1);
//   auto const e_ = t('e'+1, ~re::char_int{}, 1);
//   auto const f_ = t('f'+1, ~re::char_int{}, 1);
//
//   TEST("[^a]", rs(r(ts{_a, a_}), rf));
//   TEST("[^a-b]", rs(r(ts{_a, b_}), rf));
//   TEST("[^ab]", rs(r(ts{_a, b_}), rf));
//   TEST("[^ac]", rs(r(ts{_a, b, c_}), rf));
//   TEST("[^acd]", rs(r(ts{_a, b, d_}), rf));
//   TEST("[^ace]", rs(r(ts{_a, b, d, e_}), rf));
//   TEST("[^a-cd-f]", rs(r(ts{_a, f_}), rf));
//   TEST("[^a-ce-f]", rs(r(ts{_a, d, f_}), rf));
//
//   TEST("[ab]?", rs(r(F, ab1), rf));
//   TEST("[ab]+", rs(r(ab1), r(F, ab1)));
//   TEST("[ab]*", rs(r(F, ab1), r(F, ab1)));
//
//   auto const tc3 = t('c', 3);
//   auto const a1b2c3 = ts{ta1, tb2, tc3};
//   auto const b2c3 = ts{tb2, tc3};
//   auto const c3 = ts{tc3};
//
//   TEST("(?!ab)c", rs(r(a1), r(b2), r(c3), rf));
//   TEST("(?!ab?)c", rs(r(a1), r(b2c3), r(c3), rf));
//   TEST("(?!ab+)c", rs(r(a1), r(b2), r(b2c3), rf));
//   TEST("(?!ab*)c", rs(r(a1), r(b2c3), r(b2c3), rf));
//   TEST("(?!a?b)c", rs(r(a1b2), r(b2), r(c3), rf));
//   TEST("(?!a+b)c", rs(r(a1), r(a1b2), r(c3), rf));
//   TEST("(?!a*b)c", rs(r(a1b2), r(a1b2), r(c3), rf));
//   TEST("(?!a?b?)c", rs(r(a1b2c3), r(b2c3), r(c3), rf));
//   TEST("(?!a+b?)c", rs(r(a1), r(a1b2c3), r(c3), rf));
//   TEST("(?!a*b?)c", rs(r(a1b2c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a?b+)c", rs(r(a1b2), r(b2), r(b2c3), rf));
//   TEST("(?!a?b*)c", rs(r(a1b2c3), r(b2c3), r(b2c3), rf));
//
//   TEST("(?!ab){1}c", rs(r(a1), r(b2), r(c3), rf));
//   TEST("(?!ab?){1}c", rs(r(a1), r(b2c3), r(c3), rf));
//   TEST("(?!ab+){1}c", rs(r(a1), r(b2), r(b2c3), rf));
//   TEST("(?!ab*){1}c", rs(r(a1), r(b2c3), r(b2c3), rf));
//   TEST("(?!a?b){1}c", rs(r(a1b2), r(b2), r(c3), rf));
//   TEST("(?!a+b){1}c", rs(r(a1), r(a1b2), r(c3), rf));
//   TEST("(?!a*b){1}c", rs(r(a1b2), r(a1b2), r(c3), rf));
//   TEST("(?!a?b?){1}c", rs(r(a1b2c3), r(b2c3), r(c3), rf));
//   TEST("(?!a+b?){1}c", rs(r(a1), r(a1b2c3), r(c3), rf));
//   TEST("(?!a*b?){1}c", rs(r(a1b2c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a?b+){1}c", rs(r(a1b2), r(b2), r(b2c3), rf));
//   TEST("(?!a?b*){1}c", rs(r(a1b2c3), r(b2c3), r(b2c3), rf));
//
//   auto const tc5 = t('c', 5);
//   auto const a1b2a3 = ts{ta1, tb2, ta3};
//   auto const a1b2a3b4c5 = ts{ta1, tb2, ta3, tb4, tc5};
//   auto const a3b4c5 = ts{ta3, tb4, tc5};
//   auto const b2a3 = ts{tb2, ta3};
//   auto const b2a3b4 = ts{tb2, ta3, tb4};
//   auto const b2a3b4c5 = ts{tb2, ta3, tb4, tc5};
//   auto const b4c5 = ts{tb4, tc5};
//   auto const c5 = ts{tc5};
//
//   TEST("(?!ab){2}c", rs(r(a1), r(b2), r(a3), r(b4), r(c5), rf));
//   TEST("(?!ab?){2}c", rs(r(a1), r(b2a3), r(a3), r(b4c5), r(c5), rf));
//   TEST("(?!ab+){2}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4c5), rf));
//   TEST("(?!ab*){2}c", rs(r(a1), r(b2a3), r(b2a3), r(b4c5), r(b4c5), rf));
//   TEST("(?!a?b){2}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(c5), rf));
//   TEST("(?!a+b){2}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(c5), rf));
//   TEST("(?!a*b){2}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(c5), rf));
//   TEST("(?!a?b?){2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(a3b4c5), r(b4c5), r(c5), rf));
//   TEST("(?!a+b?){2}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4c5), r(c5), rf));
//   TEST("(?!a*b?){2}c", rs(r(a1b2a3b4c5), r(a1b2a3b4c5), r(a3b4c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a?b+){2}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4c5), rf));
//   TEST("(?!a?b*){2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(b2a3b4c5), r(b4c5), r(b4c5), rf));
//
//   auto const ta5 = t('a', 5);
//   auto const tb6 = t('b', 6);
//   auto const tc7 = t('c', 7);
//   auto const a1b2a3b4a5b6c7 = ts{ta1, tb2, ta3, tb4, ta5, tb6, tc7};
//   auto const a3b4a5 = ts{ta3, tb4, ta5};
//   auto const a3b4a5b6c7 = ts{ta3, tb4, ta5, tb6, tc7};
//   auto const a5 = ts{ta5};
//   auto const a5b6 = ts{ta5, tb6};
//   auto const a5b6c7 = ts{ta5, tb6, tc7};
//   auto const b2a3b4a5b6c7 = ts{tb2, ta3, tb4, ta5, tb6, tc7};
//   auto const b4a5 = ts{tb4, ta5};
//   auto const b4a5b6 = ts{tb4, ta5, tb6};
//   auto const b4a5b6c7 = ts{tb4, ta5, tb6, tc7};
//   auto const b6 = ts{tb6};
//   auto const b6c7 = ts{tb6, tc7};
//   auto const c7 = ts{tc7};
//
//   TEST("(?!ab){3}c", rs(r(a1), r(b2), r(a3), r(b4), r(a5), r(b6), r(c7), rf));
//   TEST("(?!ab?){3}c", rs(r(a1), r(b2a3), r(a3), r(b4a5), r(a5), r(b6c7), r(c7), rf));
//   TEST("(?!ab+){3}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4a5), r(b6), r(b6c7), rf));
//   TEST("(?!ab*){3}c", rs(r(a1), r(b2a3), r(b2a3), r(b4a5), r(b4a5), r(b6c7), r(b6c7), rf));
//   TEST("(?!a?b){3}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(a5b6), r(b6), r(c7), rf));
//   TEST("(?!a+b){3}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(a5), r(a5b6), r(c7), rf));
//   TEST("(?!a*b){3}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(a5b6), r(a5b6), r(c7), rf));
//   TEST("(?!a?b?){3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(a3b4a5b6c7), r(b4a5b6c7), r(a5b6c7), r(b6c7), r(c7), rf));
//   TEST("(?!a+b?){3}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4a5), r(a5), r(a5b6c7), r(c7), rf));
//   TEST("(?!a*b?){3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2a3b4a5b6c7), r(a3b4a5b6c7), r(a3b4a5b6c7), r(a5b6c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a?b+){3}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4a5b6), r(b6), r(b6c7), rf));
//   TEST("(?!a?b*){3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b4a5b6c7), r(b4a5b6c7), r(b6c7), r(b6c7), rf));
//
//   auto const a1c3 = ts{ta1, tc3};
//
//   TEST("(?!ab){,1}c", rs(r(a1c3), r(b2), r(c3), rf));
//   TEST("(?!ab?){,1}c", rs(r(a1c3), r(b2c3), r(c3), rf));
//   TEST("(?!ab+){,1}c", rs(r(a1c3), r(b2), r(b2c3), rf));
//   TEST("(?!ab*){,1}c", rs(r(a1c3), r(b2c3), r(b2c3), rf));
//   TEST("(?!a?b){,1}c", rs(r(a1b2c3), r(b2), r(c3), rf));
//   TEST("(?!a+b){,1}c", rs(r(a1c3), r(a1b2), r(c3), rf));
//   TEST("(?!a*b){,1}c", rs(r(a1b2c3), r(a1b2), r(c3), rf));
//   TEST("(?!a?b?){,1}c", rs(r(a1b2c3), r(b2c3), r(c3), rf));
//   TEST("(?!a+b?){,1}c", rs(r(a1c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a*b?){,1}c", rs(r(a1b2c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a?b+){,1}c", rs(r(a1b2c3), r(b2), r(b2c3), rf));
//   TEST("(?!a?b*){,1}c", rs(r(a1b2c3), r(b2c3), r(b2c3), rf));
//
//   auto const a1a3c5 = ts{ta1, ta3, tc5};
//   auto const a3c5 = ts{ta3, tc5};
//   auto const b2a3c5 = ts{tb2, ta3, tc5};
//
//   TEST("(?!ab){,2}c", rs(r(a1a3c5), r(b2), r(a3c5), r(b4), r(c5), rf));
//   TEST("(?!ab?){,2}c", rs(r(a1a3c5), r(b2a3), r(a3c5), r(b4c5), r(c5), rf));
//   TEST("(?!ab+){,2}c", rs(r(a1a3c5), r(b2), r(b2a3c5), r(b4), r(b4c5), rf));
//   TEST("(?!ab*){,2}c", rs(r(a1a3c5), r(b2a3), r(b2a3c5), r(b4c5), r(b4c5), rf));
//   TEST("(?!a?b){,2}c", rs(r(a1b2a3b4c5), r(b2), r(a3b4c5), r(b4), r(c5), rf));
//   TEST("(?!a+b){,2}c", rs(r(a1a3c5), r(a1b2), r(a3c5), r(a3b4), r(c5), rf));
//   TEST("(?!a*b){,2}c", rs(r(a1b2a3b4c5), r(a1b2), r(a3b4c5), r(a3b4), r(c5), rf));
//   TEST("(?!a?b?){,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(a3b4c5), r(b4c5), r(c5), rf));
//   TEST("(?!a+b?){,2}c", rs(r(a1a3c5), r(a1b2a3), r(a3c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a*b?){,2}c", rs(r(a1b2a3b4c5), r(a1b2a3b4c5), r(a3b4c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a?b+){,2}c", rs(r(a1b2a3b4c5), r(b2), r(b2a3b4c5), r(b4), r(b4c5), rf));
//   TEST("(?!a?b*){,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(b2a3b4c5), r(b4c5), r(b4c5), rf));
//
//   auto const a1a3a5c7 = ts{ta1, ta3, ta5, tc7};
//   auto const a1b2a3a5 = ts{ta1, tb2, ta3, ta5};
//   auto const a3a5c7 = ts{ta3, ta5, tc7};
//   auto const a3b4a5c7 = ts{ta3, tb4, ta5, tc7};
//   auto const a5c7 = ts{ta5, tc7};
//   auto const b2a3a5 = ts{tb2, ta3, ta5};
//   auto const b2a3a5c7 = ts{tb2, ta3, ta5, tc7};
//   auto const b4a5c7 = ts{tb4, ta5, tc7};
//
//   TEST("(?!ab){,3}c", rs(r(a1a3a5c7), r(b2), r(a3a5c7), r(b4), r(a5c7), r(b6), r(c7), rf));
//   TEST("(?!ab?){,3}c", rs(r(a1a3a5c7), r(b2a3a5), r(a3a5c7), r(b4a5c7), r(a5c7), r(b6c7), r(c7), rf));
//   TEST("(?!ab+){,3}c", rs(r(a1a3a5c7), r(b2), r(b2a3a5c7), r(b4), r(b4a5c7), r(b6), r(b6c7), rf));
//   TEST("(?!ab*){,3}c", rs(r(a1a3a5c7), r(b2a3a5), r(b2a3a5c7), r(b4a5c7), r(b4a5c7), r(b6c7), r(b6c7), rf));
//   TEST("(?!a?b){,3}c", rs(r(a1b2a3b4a5b6c7), r(b2), r(a3b4a5b6c7), r(b4), r(a5b6c7), r(b6), r(c7), rf));
//   TEST("(?!a+b){,3}c", rs(r(a1a3a5c7), r(a1b2), r(a3a5c7), r(a3b4), r(a5c7), r(a5b6), r(c7), rf));
//   TEST("(?!a*b){,3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2), r(a3b4a5b6c7), r(a3b4), r(a5b6c7), r(a5b6), r(c7), rf));
//   TEST("(?!a?b?){,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(a3b4a5b6c7), r(b4a5b6c7), r(a5b6c7), r(b6c7), r(c7), rf));
//   TEST("(?!a+b?){,3}c", rs(r(a1a3a5c7), r(a1b2a3a5), r(a3a5c7), r(a3b4a5c7), r(a5c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a*b?){,3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2a3b4a5b6c7), r(a3b4a5b6c7), r(a3b4a5b6c7), r(a5b6c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a?b+){,3}c", rs(r(a1b2a3b4a5b6c7), r(b2), r(b2a3b4a5b6c7), r(b4), r(b4a5b6c7), r(b6), r(b6c7), rf));
//   TEST("(?!a?b*){,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b4a5b6c7), r(b4a5b6c7), r(b6c7), r(b6c7), rf));
//
//   auto const a1b2a1b2c3 = ts{ta1, tb2, ta1, tb2, tc3};
//   auto const a1b2a1c3 = ts{ta1, tb2, ta1, tc3};
//   auto const b2a1b2c3 = ts{tb2, ta1, tb2, tc3};
//   auto const b2a1c3 = ts{tb2, ta1, tc3};
//
//   TEST("(?!ab){1,}c", rs(r(a1), r(b2), r(a1c3), rf));
//   TEST("(?!ab?){1,}c", rs(r(a1), r(b2a1c3), r(a1c3), rf));
//   TEST("(?!ab+){1,}c", rs(r(a1), r(b2), r(b2a1c3), rf));
//   TEST("(?!ab*){1,}c", rs(r(a1), r(b2a1c3), r(b2a1c3), rf));
//   TEST("(?!a?b){1,}c", rs(r(a1b2), r(b2), r(a1b2c3), rf));
//   TEST("(?!a+b){1,}c", rs(r(a1), r(a1b2), r(a1c3), rf));
//   TEST("(?!a*b){1,}c", rs(r(a1b2), r(a1b2), r(a1b2c3), rf));
//   TEST("(?!a?b?){1,}c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a+b?){1,}c", rs(r(a1), r(a1b2a1c3), r(a1c3), rf));
//   TEST("(?!a*b?){1,}c", rs(r(a1b2a1b2c3), r(a1b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a?b+){1,}c", rs(r(a1b2), r(b2), r(b2a1b2c3), rf));
//   TEST("(?!a?b*){1,}c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(b2a1b2c3), rf));
//
//   auto const a1b2a3b4a3b4c5 = ts{ta1, tb2, ta3, tb4, ta3, tb4, tc5};
//   auto const a3b4a3b4c5 = ts{ta3, tb4, ta3, tb4, tc5};
//   auto const a3b4a3c5 = ts{ta3, tb4, ta3, tc5};
//   auto const b2a3b4a3b4c5 = ts{tb2, ta3, tb4, ta3, tb4, tc5};
//   auto const b4a3b4c5 = ts{tb4, ta3, tb4, tc5};
//   auto const b4a3c5 = ts{tb4, ta3, tc5};
//
//   TEST("(?!ab){2,}c", rs(r(a1), r(b2), r(a3), r(b4), r(a3c5), rf));
//   TEST("(?!ab?){2,}c", rs(r(a1), r(b2a3), r(a3), r(b4a3c5), r(a3c5), rf));
//   TEST("(?!ab+){2,}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4a3c5), rf));
//   TEST("(?!ab*){2,}c", rs(r(a1), r(b2a3), r(b2a3), r(b4a3c5), r(b4a3c5), rf));
//   TEST("(?!a?b){2,}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(a3b4c5), rf));
//   TEST("(?!a+b){2,}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(a3c5), rf));
//   TEST("(?!a*b){2,}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(a3b4c5), rf));
//   TEST("(?!a?b?){2,}c", rs(r(a1b2a3b4a3b4c5), r(b2a3b4a3b4c5), r(a3b4a3b4c5), r(b4a3b4c5), r(a3b4c5), rf));
//   TEST("(?!a+b?){2,}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4a3c5), r(a3c5), rf));
//   TEST("(?!a*b?){2,}c", rs(r(a1b2a3b4a3b4c5), r(a1b2a3b4a3b4c5), r(a3b4a3b4c5), r(a3b4a3b4c5), r(a3b4c5), rf));
//   TEST("(?!a?b+){2,}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4a3b4c5), rf));
//   TEST("(?!a?b*){2,}c", rs(r(a1b2a3b4a3b4c5), r(b2a3b4a3b4c5), r(b2a3b4a3b4c5), r(b4a3b4c5), r(b4a3b4c5), rf));
//
//   auto const a1b2a3b4a5b6a5b6c7 = ts{ta1, tb2, ta3, tb4, ta5, tb6, ta5, tb6, tc7};
//   auto const a3b4a5b6a5b6c7 = ts{ta3, tb4, ta5, tb6, ta5, tb6, tc7};
//   auto const a5b6a5b6c7 = ts{ta5, tb6, ta5, tb6, tc7};
//   auto const a5b6a5c7 = ts{ta5, tb6, ta5, tc7};
//   auto const b2a3b4a5b6a5b6c7 = ts{tb2, ta3, tb4, ta5, tb6, ta5, tb6, tc7};
//   auto const b4a5b6a5b6c7 = ts{tb4, ta5, tb6, ta5, tb6, tc7};
//   auto const b6a5b6c7 = ts{tb6, ta5, tb6, tc7};
//   auto const b6a5c7 = ts{tb6, ta5, tc7};
//
//   TEST("(?!ab){3,}c", rs(r(a1), r(b2), r(a3), r(b4), r(a5), r(b6), r(a5c7), rf));
//   TEST("(?!ab?){3,}c", rs(r(a1), r(b2a3), r(a3), r(b4a5), r(a5), r(b6a5c7), r(a5c7), rf));
//   TEST("(?!ab+){3,}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4a5), r(b6), r(b6a5c7), rf));
//   TEST("(?!ab*){3,}c", rs(r(a1), r(b2a3), r(b2a3), r(b4a5), r(b4a5), r(b6a5c7), r(b6a5c7), rf));
//   TEST("(?!a?b){3,}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(a5b6), r(b6), r(a5b6c7), rf));
//   TEST("(?!a+b){3,}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(a5), r(a5b6), r(a5c7), rf));
//   TEST("(?!a*b){3,}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(a5b6), r(a5b6), r(a5b6c7), rf));
//   TEST("(?!a?b?){3,}c", rs(r(a1b2a3b4a5b6a5b6c7), r(b2a3b4a5b6a5b6c7), r(a3b4a5b6a5b6c7), r(b4a5b6a5b6c7), r(a5b6a5b6c7), r(b6a5b6c7), r(a5b6c7), rf));
//   TEST("(?!a+b?){3,}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4a5), r(a5), r(a5b6a5c7), r(a5c7), rf));
//   TEST("(?!a*b?){3,}c", rs(r(a1b2a3b4a5b6a5b6c7), r(a1b2a3b4a5b6a5b6c7), r(a3b4a5b6a5b6c7), r(a3b4a5b6a5b6c7), r(a5b6a5b6c7), r(a5b6a5b6c7), r(a5b6c7), rf));
//   TEST("(?!a?b+){3,}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4a5b6), r(b6), r(b6a5b6c7), rf));
//   TEST("(?!a?b*){3,}c", rs(r(a1b2a3b4a5b6a5b6c7), r(b2a3b4a5b6a5b6c7), r(b2a3b4a5b6a5b6c7), r(b4a5b6a5b6c7), r(b4a5b6a5b6c7), r(b6a5b6c7), r(b6a5b6c7), rf));
//
//   TEST("(?!ab){1,1}c", rs(r(a1), r(b2), r(c3), rf));
//   TEST("(?!ab?){1,1}c", rs(r(a1), r(b2c3), r(c3), rf));
//   TEST("(?!ab+){1,1}c", rs(r(a1), r(b2), r(b2c3), rf));
//   TEST("(?!ab*){1,1}c", rs(r(a1), r(b2c3), r(b2c3), rf));
//   TEST("(?!a?b){1,1}c", rs(r(a1b2), r(b2), r(c3), rf));
//   TEST("(?!a+b){1,1}c", rs(r(a1), r(a1b2), r(c3), rf));
//   TEST("(?!a*b){1,1}c", rs(r(a1b2), r(a1b2), r(c3), rf));
//   TEST("(?!a?b?){1,1}c", rs(r(a1b2c3), r(b2c3), r(c3), rf));
//   TEST("(?!a+b?){1,1}c", rs(r(a1), r(a1b2c3), r(c3), rf));
//   TEST("(?!a*b?){1,1}c", rs(r(a1b2c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a?b+){1,1}c", rs(r(a1b2), r(b2), r(b2c3), rf));
//   TEST("(?!a?b*){1,1}c", rs(r(a1b2c3), r(b2c3), r(b2c3), rf));
//
//   auto const a1b2a3c5 = ts{ta1, tb2, ta3, tc5};
//
//   TEST("(?!ab){1,2}c", rs(r(a1), r(b2), r(a3c5), r(b4), r(c5), rf));
//   TEST("(?!ab?){1,2}c", rs(r(a1), r(b2a3c5), r(a3c5), r(b4c5), r(c5), rf));
//   TEST("(?!ab+){1,2}c", rs(r(a1), r(b2), r(b2a3c5), r(b4), r(b4c5), rf));
//   TEST("(?!ab*){1,2}c", rs(r(a1), r(b2a3c5), r(b2a3c5), r(b4c5), r(b4c5), rf));
//   TEST("(?!a?b){1,2}c", rs(r(a1b2), r(b2), r(a3b4c5), r(b4), r(c5), rf));
//   TEST("(?!a+b){1,2}c", rs(r(a1), r(a1b2), r(a3c5), r(a3b4), r(c5), rf));
//   TEST("(?!a*b){1,2}c", rs(r(a1b2), r(a1b2), r(a3b4c5), r(a3b4), r(c5), rf));
//   TEST("(?!a?b?){1,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(a3b4c5), r(b4c5), r(c5), rf));
//   TEST("(?!a+b?){1,2}c", rs(r(a1), r(a1b2a3c5), r(a3c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a*b?){1,2}c", rs(r(a1b2a3b4c5), r(a1b2a3b4c5), r(a3b4c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a?b+){1,2}c", rs(r(a1b2), r(b2), r(b2a3b4c5), r(b4), r(b4c5), rf));
//   TEST("(?!a?b*){1,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(b2a3b4c5), r(b4c5), r(b4c5), rf));
//
//   auto const a1b2a3c7 = ts{ta1, tb2, ta3, tc7};
//   auto const a3b4c7 = ts{ta3, tb4, tc7};
//   auto const a3c7 = ts{ta3, tc7};
//   auto const b2a3b4c7 = ts{tb2, ta3, tb4, tc7};
//   auto const b2a3c7 = ts{tb2, ta3, tc7};
//
//   TEST("(?!ab){1,3}c", rs(r(a1), r(b2), r(a3c7), r(b4), r(a5c7), r(b6), r(c7), rf));
//   TEST("(?!ab?){1,3}c", rs(r(a1), r(b2a3c7), r(a3c7), r(b4a5c7), r(a5c7), r(b6c7), r(c7), rf));
//   TEST("(?!ab+){1,3}c", rs(r(a1), r(b2), r(b2a3c7), r(b4), r(b4a5c7), r(b6), r(b6c7), rf));
//   TEST("(?!ab*){1,3}c", rs(r(a1), r(b2a3c7), r(b2a3c7), r(b4a5c7), r(b4a5c7), r(b6c7), r(b6c7), rf));
//   TEST("(?!a?b){1,3}c", rs(r(a1b2), r(b2), r(a3b4c7), r(b4), r(a5b6c7), r(b6), r(c7), rf));
//   TEST("(?!a+b){1,3}c", rs(r(a1), r(a1b2), r(a3c7), r(a3b4), r(a5c7), r(a5b6), r(c7), rf));
//   TEST("(?!a*b){1,3}c", rs(r(a1b2), r(a1b2), r(a3b4c7), r(a3b4), r(a5b6c7), r(a5b6), r(c7), rf));
//   TEST("(?!a?b?){1,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(a3b4a5b6c7), r(b4a5b6c7), r(a5b6c7), r(b6c7), r(c7), rf));
//   TEST("(?!a+b?){1,3}c", rs(r(a1), r(a1b2a3c7), r(a3c7), r(a3b4a5c7), r(a5c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a*b?){1,3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2a3b4a5b6c7), r(a3b4a5b6c7), r(a3b4a5b6c7), r(a5b6c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a?b+){1,3}c", rs(r(a1b2), r(b2), r(b2a3b4c7), r(b4), r(b4a5b6c7), r(b6), r(b6c7), rf));
//   TEST("(?!a?b*){1,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b4a5b6c7), r(b4a5b6c7), r(b6c7), r(b6c7), rf));
//
//   TEST("(?!ab){2,2}c", rs(r(a1), r(b2), r(a3), r(b4), r(c5), rf));
//   TEST("(?!ab?){2,2}c", rs(r(a1), r(b2a3), r(a3), r(b4c5), r(c5), rf));
//   TEST("(?!ab+){2,2}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4c5), rf));
//   TEST("(?!ab*){2,2}c", rs(r(a1), r(b2a3), r(b2a3), r(b4c5), r(b4c5), rf));
//   TEST("(?!a?b){2,2}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(c5), rf));
//   TEST("(?!a+b){2,2}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(c5), rf));
//   TEST("(?!a*b){2,2}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(c5), rf));
//   TEST("(?!a?b?){2,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(a3b4c5), r(b4c5), r(c5), rf));
//   TEST("(?!a+b?){2,2}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4c5), r(c5), rf));
//   TEST("(?!a*b?){2,2}c", rs(r(a1b2a3b4c5), r(a1b2a3b4c5), r(a3b4c5), r(a3b4c5), r(c5), rf));
//   TEST("(?!a?b+){2,2}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4c5), rf));
//   TEST("(?!a?b*){2,2}c", rs(r(a1b2a3b4c5), r(b2a3b4c5), r(b2a3b4c5), r(b4c5), r(b4c5), rf));
//
//   TEST("(?!ab){2,3}c", rs(r(a1), r(b2), r(a3), r(b4), r(a5c7), r(b6), r(c7), rf));
//   TEST("(?!ab?){2,3}c", rs(r(a1), r(b2a3), r(a3), r(b4a5c7), r(a5c7), r(b6c7), r(c7), rf));
//   TEST("(?!ab+){2,3}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4a5c7), r(b6), r(b6c7), rf));
//   TEST("(?!ab*){2,3}c", rs(r(a1), r(b2a3), r(b2a3), r(b4a5c7), r(b4a5c7), r(b6c7), r(b6c7), rf));
//   TEST("(?!a?b){2,3}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(a5b6c7), r(b6), r(c7), rf));
//   TEST("(?!a+b){2,3}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(a5c7), r(a5b6), r(c7), rf));
//   TEST("(?!a*b){2,3}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(a5b6c7), r(a5b6), r(c7), rf));
//   TEST("(?!a?b?){2,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(a3b4a5b6c7), r(b4a5b6c7), r(a5b6c7), r(b6c7), r(c7), rf));
//   TEST("(?!a+b?){2,3}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4a5c7), r(a5c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a*b?){2,3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2a3b4a5b6c7), r(a3b4a5b6c7), r(a3b4a5b6c7), r(a5b6c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a?b+){2,3}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4a5b6c7), r(b6), r(b6c7), rf));
//   TEST("(?!a?b*){2,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b4a5b6c7), r(b4a5b6c7), r(b6c7), r(b6c7), rf));
//
//   TEST("(?!ab){3,3}c", rs(r(a1), r(b2), r(a3), r(b4), r(a5), r(b6), r(c7), rf));
//   TEST("(?!ab?){3,3}c", rs(r(a1), r(b2a3), r(a3), r(b4a5), r(a5), r(b6c7), r(c7), rf));
//   TEST("(?!ab+){3,3}c", rs(r(a1), r(b2), r(b2a3), r(b4), r(b4a5), r(b6), r(b6c7), rf));
//   TEST("(?!ab*){3,3}c", rs(r(a1), r(b2a3), r(b2a3), r(b4a5), r(b4a5), r(b6c7), r(b6c7), rf));
//   TEST("(?!a?b){3,3}c", rs(r(a1b2), r(b2), r(a3b4), r(b4), r(a5b6), r(b6), r(c7), rf));
//   TEST("(?!a+b){3,3}c", rs(r(a1), r(a1b2), r(a3), r(a3b4), r(a5), r(a5b6), r(c7), rf));
//   TEST("(?!a*b){3,3}c", rs(r(a1b2), r(a1b2), r(a3b4), r(a3b4), r(a5b6), r(a5b6), r(c7), rf));
//   TEST("(?!a?b?){3,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(a3b4a5b6c7), r(b4a5b6c7), r(a5b6c7), r(b6c7), r(c7), rf));
//   TEST("(?!a+b?){3,3}c", rs(r(a1), r(a1b2a3), r(a3), r(a3b4a5), r(a5), r(a5b6c7), r(c7), rf));
//   TEST("(?!a*b?){3,3}c", rs(r(a1b2a3b4a5b6c7), r(a1b2a3b4a5b6c7), r(a3b4a5b6c7), r(a3b4a5b6c7), r(a5b6c7), r(a5b6c7), r(c7), rf));
//   TEST("(?!a?b+){3,3}c", rs(r(a1b2), r(b2), r(b2a3b4), r(b4), r(b4a5b6), r(b6), r(b6c7), rf));
//   TEST("(?!a?b*){3,3}c", rs(r(a1b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b2a3b4a5b6c7), r(b4a5b6c7), r(b4a5b6c7), r(b6c7), r(b6c7), rf));
//
//   TEST("(?!ab)?c", rs(r(a1c3), r(b2), r(c3), rf));
//   TEST("(?!ab?)?c", rs(r(a1c3), r(b2c3), r(c3), rf));
//   TEST("(?!ab+)?c", rs(r(a1c3), r(b2), r(b2c3), rf));
//   TEST("(?!ab*)?c", rs(r(a1c3), r(b2c3), r(b2c3), rf));
//   TEST("(?!a?b)?c", rs(r(a1b2c3), r(b2), r(c3), rf));
//   TEST("(?!a+b)?c", rs(r(a1c3), r(a1b2), r(c3), rf));
//   TEST("(?!a*b)?c", rs(r(a1b2c3), r(a1b2), r(c3), rf));
//   TEST("(?!a?b?)?c", rs(r(a1b2c3), r(b2c3), r(c3), rf));
//   TEST("(?!a+b?)?c", rs(r(a1c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a*b?)?c", rs(r(a1b2c3), r(a1b2c3), r(c3), rf));
//   TEST("(?!a?b+)?c", rs(r(a1b2c3), r(b2), r(b2c3), rf));
//   TEST("(?!a?b*)?c", rs(r(a1b2c3), r(b2c3), r(b2c3), rf));
//
//   TEST("(?!ab)+c", rs(r(a1), r(b2), r(a1c3), rf));
//   TEST("(?!ab?)+c", rs(r(a1), r(b2a1c3), r(a1c3), rf));
//   TEST("(?!ab+)+c", rs(r(a1), r(b2), r(b2a1c3), rf));
//   TEST("(?!ab*)+c", rs(r(a1), r(b2a1c3), r(b2a1c3), rf));
//   TEST("(?!a?b)+c", rs(r(a1b2), r(b2), r(a1b2c3), rf));
//   TEST("(?!a+b)+c", rs(r(a1), r(a1b2), r(a1c3), rf));
//   TEST("(?!a*b)+c", rs(r(a1b2), r(a1b2), r(a1b2c3), rf));
//   TEST("(?!a?b?)+c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a+b?)+c", rs(r(a1), r(a1b2a1c3), r(a1c3), rf));
//   TEST("(?!a*b?)+c", rs(r(a1b2a1b2c3), r(a1b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a?b+)+c", rs(r(a1b2), r(b2), r(b2a1b2c3), rf));
//   TEST("(?!a?b*)+c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(b2a1b2c3), rf));
//
//   TEST("(?!ab)*c", rs(r(a1c3), r(b2), r(a1c3), rf));
//   TEST("(?!ab?)*c", rs(r(a1c3), r(b2a1c3), r(a1c3), rf));
//   TEST("(?!ab+)*c", rs(r(a1c3), r(b2), r(b2a1c3), rf));
//   TEST("(?!ab*)*c", rs(r(a1c3), r(b2a1c3), r(b2a1c3), rf));
//   TEST("(?!a?b)*c", rs(r(a1b2c3), r(b2), r(a1b2c3), rf));
//   TEST("(?!a+b)*c", rs(r(a1c3), r(a1b2), r(a1c3), rf));
//   TEST("(?!a*b)*c", rs(r(a1b2c3), r(a1b2), r(a1b2c3), rf));
//   TEST("(?!a?b?)*c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a+b?)*c", rs(r(a1c3), r(a1b2a1c3), r(a1c3), rf));
//   TEST("(?!a*b?)*c", rs(r(a1b2a1b2c3), r(a1b2a1b2c3), r(a1b2c3), rf));
//   TEST("(?!a?b+)*c", rs(r(a1b2c3), r(b2), r(b2a1b2c3), rf));
//   TEST("(?!a?b*)*c", rs(r(a1b2a1b2c3), r(b2a1b2c3), r(b2a1b2c3), rf));
//
//   TEST("a?(?!b)c", rs(r(a1b2), r(b2), r(c3), rf));
//   TEST("a+(?!b)c", rs(r(a1), r(a1b2), r(c3), rf));
//   TEST("a*(?!b)c", rs(r(a1a1b2), r(a1b2), r(c3), rf));
//
//   auto const tc4 = t('c', 4);
//   auto const te5 = t('e', 5);
//   auto const td4 = t('d', 4);
//   auto const a1c3e5 = ts{ta1, tc3, te5};
//   auto const a2b2c3e5 = ts{ta2, tb2, tc3, te5};
//   auto const a2b2c4d4e5 = ts{ta2, tb2, tc4, td4, te5};
//   auto const b2c3e5 = ts{tb2, tc3, te5};
//   auto const c3e5 = ts{tc3, te5};
//   auto const c4d4e5 = ts{tc4, td4, te5};
//   auto const d4 = ts{td4};
//   auto const e5 = ts{te5};
//
//   TEST("(?!ab)?(?!cd)?e", rs(r(a1c3e5), r(b2), r(c3e5), r(d4), r(e5), rf));
//   TEST("(?!a|b)?(?!c?|d)?e", rs(r(a2b2c4d4e5), r(b2), r(c4d4e5), r(d4), r(e5), rf));
//   TEST("(?!a?|b)?(?!cd)?e", rs(r(a2b2c3e5), r(b2), r(c3e5), r(d4), r(e5), rf));
//   TEST("(?!a?|b?)?(?!cd)?e", rs(r(a2b2c3e5), r(b2c3e5), r(c3e5), r(d4), r(e5), rf));;
//   TEST("(?!ab){,1}(?!cd){,1}e", rs(r(a1c3e5), r(b2), r(c3e5), r(d4), r(e5), rf));
//   TEST("(?!a|b){,1}(?!c?|d){,1}e", rs(r(a2b2c4d4e5), r(b2), r(c4d4e5), r(d4), r(e5), rf));
//   TEST("(?!a?|b){,1}(?!cd){,1}e", rs(r(a2b2c3e5), r(b2), r(c3e5), r(d4), r(e5), rf));
//   TEST("(?!a?|b?){,1}(?!cd){,1}e", rs(r(a2b2c3e5), r(b2c3e5), r(c3e5), r(d4), r(e5), rf));
//
//   auto const a2b2 = ts{ta2, tb2};
//
//   TEST("a|b", rs(r(a2b2), none, rf));
//   TEST("a|b?", rs(r(F, a2b2), none, rf));
//
//   auto const td5 = t('d', 5);
//   auto const td6 = t('d', 6);
//   auto const a1b3c5 = ts{ta1, tb3, tc5};
//   auto const a1b4c4 = ts{ta1, tb4, tc4};
//   auto const a1d5 = ts{ta1, td5};
//   auto const a1d6 = ts{ta1, td6};
//   auto const a3b3c3 = ts{ta3, tb3, tc3};
//   auto const b3c3 = ts{tb3, tc3};
//   auto const b3d6 = ts{tb3, td6};
//   auto const c4 = ts{tc4};
//   auto const d5 = ts{td5};
//   auto const d6 = ts{td6};
//
//   TEST("(?!a+|(?!b+|c))d", rs(r(a1b3c5), r(a1d6), none, r(b3d6), none, r(d6), rf));
//   TEST("(?!a+|(?!b|c))d", rs(r(a1b4c4), r(a1d5), none, r(c4), r(d5), rf));
//   TEST("(?!a|(?!b|c))d", rs(r(a3b3c3), r(b3c3), r(c3), r(d4), rf));
//
//   auto const a2b2b2 = ts{ta2, tb2, tb2};
//   auto const a2b2b2c3 = ts{ta2, tb2, tb2, tc3};
//
//   TEST("a|", rs(r(F, a1), rf));
//   TEST("|a", rs(r(F, a1), rf));
//   TEST("a||b", rs(r(F, a2b2b2), r(b2), rf));
//   TEST("(?!)a", rs(r(a1), rf));
//   TEST("(?!a|)b", rs(r(a1b2), r(b2), rf));
//   TEST("(?!|a)b", rs(r(a1b2), r(b2), rf));
//   TEST("(?!a||b)c", rs(r(a2b2b2c3), r(b2), r(c3), rf));
//
//   auto const tc3B = tB(tc3);
//   auto const a2b2Bc3B = ts{ta2, tB(tb2), tc3B};
//   auto const b2Bc3B = ts{tB(tb2), tc3B};
//
//   TEST("(?!a|^b?)c", rs(r(a2b2Bc3B), r(b2Bc3B), r(c3), rf));
//
//   auto const a2Bb2B = ts{tB(ta2), tB(tb2)};
//   auto const b2B = ts{tB(tb2)};
//
//   TEST("^(?!a|b)", rs(r(a2Bb2B), none, rf));

  if (count_test_failure) {
    std::cerr << "error(s): " << count_test_failure << " / " << count_test << "\n";
  }
  return count_test_failure ? 1 : 0;
}
