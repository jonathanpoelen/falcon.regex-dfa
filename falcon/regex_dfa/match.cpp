#include "match.hpp"
#include "redfa.hpp"
#include "regex_consumer.hpp"

// TODO trace.h + FALCON_REGEX_DFA_ENABLE_TRACE != 0
#if defined(FALCON_REGEX_DFA_ENABLE_TRACE) || defined(IN_IDE_PARSER)
# define FALCON_REGEX_DFA_TRACE void
# include "print_automaton.hpp"
# include <iostream>
#else
# define FALCON_REGEX_DFA_TRACE(...)
#endif

#include <memory>


namespace falcon { namespace regex_dfa {

bool match(const Ranges& rngs, const char* s)
{
  if (rngs.empty()) {
    return true;
  }

  FALCON_REGEX_DFA_TRACE(std::cerr << "# match:\n");

  std::size_t i = 0;
  utf8_consumer consumer(s);
  char_int c;
  bool final;
  while (
      !(final = bool(rngs[i].states & Range::Final))
    && (c = consumer.bumpc())
    //&& (rngs[i].states & Range::Normal)
    && ([&]() -> bool {
      FALCON_REGEX_DFA_TRACE(std::cerr << "--- " << utf8_char(c) << " ---\n");
      FALCON_REGEX_DFA_TRACE(print_automaton(rngs[i], int(i)));
      for (auto && t : rngs[i].transitions) {
        if (t.e.contains(c)) {
          i = t.next;
          return true;
        }
      }
      return false;
    }())
  );

  FALCON_REGEX_DFA_TRACE(std::cerr
    << "final: " << final
    << "\nc: " << c
    << "\nend: " << bool(rngs[i].states & Range::End)
    << "\n"
  );
  return final || (!c && bool(rngs[i].states & Range::End));
}


bool nfa_match(const Ranges& rngs, const char* s)
{
  if (rngs.empty()) {
    return true;
  }

  FALCON_REGEX_DFA_TRACE(std::cerr << "# nfa_match:\n");

  struct DynArray {
    using value_type = Range const *;
    std::unique_ptr<value_type> a;
    value_type * p;

    DynArray(std::size_t sz): a(new value_type[sz]), p(a.get()) {}

    value_type * begin() const { return a.get(); }
    value_type * end() const { return p; }
    void push_back(Range const & rng) { *p = &rng; ++p; }
    bool empty() const { return p == a.get(); }
    void clear() { p = a.get(); }
  };

  std::vector<unsigned> crossing_table(rngs.size(), 0);
  DynArray t1(rngs.size());
  DynArray t2(rngs.size());

  t1.push_back(rngs.front());

  auto has_state = [&t1](Range::State e) {
    for (Range const * prng : t1) {
      if (bool(prng->states & e)) {
        return true;
      }
    }
    return false;
  };

  unsigned auto_increment = 1;
  utf8_consumer consumer(s);
  char_int c;
  bool final;
  while (
      !(final = has_state(Range::Final))
    && (c = consumer.bumpc())
    && ([&]() -> bool {
      for (Range const * prng : t1) {
        FALCON_REGEX_DFA_TRACE(std::cerr << "--- " << utf8_char(c) << " ---\n");
        FALCON_REGEX_DFA_TRACE(print_automaton(*prng, int(prng-&rngs.front())));
        for (auto && t : prng->transitions) {
          if (t.e.contains(c) && crossing_table[t.next] < auto_increment) {
            t2.push_back(rngs[t.next]);
            crossing_table[t.next] = auto_increment;
          }
        }
      }
      using std::swap;
      swap(t1, t2);
      t2.clear();
      ++auto_increment;
      return !t1.empty();
    }())
  );

  FALCON_REGEX_DFA_TRACE(std::cerr
    << "final: " << final
    << "\nc: " << c
    << "\nend: " << has_state(Range::End)
    << "\n"
  );
  return final || (!c && has_state(Range::End));
}


} }
