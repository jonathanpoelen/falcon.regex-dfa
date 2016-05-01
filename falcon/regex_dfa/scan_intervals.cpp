#include "scan_intervals.hpp"
#include "reverse_transitions.hpp"
#include "redfa.hpp"

#include <stdexcept>
#include <algorithm>

void falcon::regex_dfa::scan_intervals(
  utf8_consumer& consumer, Transitions& ts, unsigned int next_ts, Transition::State state
) {
  char_int c = consumer.bumpc();
  bool const reverse = [&]() -> bool {
    if (c == '^') {
      c = consumer.bumpc();
      return true;
    }
    return false;
  }();

  if (c == '-') {
    ts.push_back({{c, c}, next_ts, state});
    c = consumer.bumpc();
  }

  while (c && c != ']') {
    if (c == '-') {
      if (!(c = consumer.bumpc())) {
        throw std::runtime_error("missing terminating ]");
      }

      if (c == ']') {
        ts.push_back({{'-', '-'}, next_ts, state});
      }
      else {
        Event & e = ts.back().e;
        if (e.l != e.r) {
          ts.push_back({{'-', '-'}, next_ts, state});
        }
        else if (!(
            (('0' <= e.l && e.l <= '9' && '0' <= c && c <= '9')
          || ('a' <= e.l && e.l <= 'z' && 'a' <= c && c <= 'z')
          || ('A' <= e.l && e.l <= 'Z' && 'A' <= c && c <= 'Z')
          ) && e.l <= c
        )) {
          throw std::runtime_error("range out of order in character class");
        }
        else {
          e.r = c;
          c = consumer.bumpc();
        }
      }
    }
    else {
      if (c == '\\') {
        if (!(c = consumer.bumpc())) {
          throw std::runtime_error("missing terminating ]");
        }
      }

      ts.push_back({{c, c}, next_ts, state});
      c = consumer.bumpc();
    }
  }

  if (!c) {
    throw std::runtime_error("missing terminating ]");
  }

  if (ts.empty()) {
    throw std::runtime_error("error end of range");
  }

  if (reverse) {
    reverse_transitions(ts, next_ts, state);
  }
}
