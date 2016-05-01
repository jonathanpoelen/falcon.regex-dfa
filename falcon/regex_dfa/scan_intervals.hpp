#ifndef FALCON_REGEX_DFA_REGEX_SCAN_INTERVALS_HPP
#define FALCON_REGEX_DFA_REGEX_SCAN_INTERVALS_HPP

#include "redfa.hpp"

namespace falcon { namespace regex_dfa {
  class utf8_consumer;
  void scan_intervals(
    utf8_consumer & consumer,
    Transitions & ts,
    unsigned next_ts,
    Transition::State state
  );
} }

#endif
