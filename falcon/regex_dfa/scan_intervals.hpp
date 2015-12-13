#ifndef FALCON_REGEX_DFA_REGEX_SCAN_INTERVALS_HPP
#define FALCON_REGEX_DFA_REGEX_SCAN_INTERVALS_HPP

namespace falcon { namespace regex_dfa {
  class utf8_consumer;
  class Transitions;
  void scan_intervals(utf8_consumer & consumer, Transitions & ts, unsigned next_ts);
} }

#endif
