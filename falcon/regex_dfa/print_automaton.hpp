#ifndef FALCON_REGEX_DFA_PRINT_AUTOMATON_HPP
#define FALCON_REGEX_DFA_PRINT_AUTOMATON_HPP

namespace falcon { namespace regex_dfa {
  class Range;
  class Ranges;
  class Transition;
  class Transitions;

  void print_automaton(Ranges const &);
  void print_automaton(Range const &, int num = -1);
  void print_automaton(Transitions const &);
  void print_automaton(Transition const &);
} }

#endif
