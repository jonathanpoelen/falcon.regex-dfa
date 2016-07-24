#include "match.hpp"
#include "redfa.hpp"
#include "regex_consumer.hpp"
#include "trace.hpp"

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
    << "\nend: " << bool(rngs[i].states & Range::Eol)
    << "\n"
  );
  return final || (!c && bool(rngs[i].states & Range::Eol));
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

  std::unique_ptr<unsigned[]> crossing_table(new unsigned[rngs.size()]{});
  DynArray t1(rngs.size());
  DynArray t2(rngs.size());

  t1.push_back(rngs.front());

  unsigned auto_increment = 1;
  utf8_consumer consumer(s);
  char_int c;

  auto next = [&](Range::State states){
    for (Range const * prng : t1) {
      FALCON_REGEX_DFA_TRACE(std::cerr << "--- " << utf8_char(c) << " ---\n");
      FALCON_REGEX_DFA_TRACE(print_automaton(*prng, int(prng-&rngs.front())));
      if (bool(prng->states & states)) {
        for (auto && t : prng->transitions) {
          if (t.e.contains(c)
          && crossing_table[t.next] < auto_increment
          ) {
            t2.push_back(rngs[t.next]);
            crossing_table[t.next] = auto_increment;
          }
        }
      }
    }

    using std::swap;
    swap(t1, t2);
    t2.clear();
    ++auto_increment;
  };

  if ((c = consumer.bumpc()) && !t1.empty()) {
    next(Range::Normal | Range::Bol);

    while ((c = consumer.bumpc()) && !t1.empty()) {
      next(Range::Normal);
    };
  }

  auto has_state = [&t1](Range::State e) {
    for (Range const * prng : t1) {
      if (bool(prng->states & e)) {
        return true;
      }
    }
    return false;
  };

  FALCON_REGEX_DFA_TRACE(std::cerr
    << "final: " << has_state(Range::Final)
    << "\nc: " << c
    << "\n"
  );
  return (c ? has_state(Range::Final) : has_state(Range::Final | Range::Eol));
}


} }
