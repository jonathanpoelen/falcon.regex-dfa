#ifndef FALCON_REGEX_DFA_REDFA_HPP
#define FALCON_REGEX_DFA_REDFA_HPP

#include "regex_consumer.hpp"

#include <vector>


namespace falcon { namespace regex_dfa {

struct Event
{
  char_int l, r;

  bool is_char() const { return l == r; }

  bool operator < (Event const & other) const {
    return l < other.l
        || (l == other.l && r < other.r);
  }

  bool operator == (Event const & other) const {
    return l == other.l
        && r == other.r;
  }

  bool contains(char_int c) const noexcept {
    return l <= c && c <= r;
  }
};

struct RangeTransition;

struct Transition
{
  Event e;
  std::size_t next;

  bool operator < (Transition const & other) const {
    return e < other.e || (e == other.e && next < other.next);
  }

  bool operator == (Transition const & other) const {
    return next == other.next
        && e == other.e;
  }
};

struct Transitions : std::vector<Transition> {
  using std::vector<Transition>::vector;
};

struct Capture {
  unsigned n;
  enum State {
    None,
    Open   = 1 << 0,
    Close  = 1 << 1,
    Active = 1 << 2,
    Exists = 1 << 3,
    NotExists = 1 << 4,
    MAX = 1 << 5,
  } e;

  bool operator < (Capture const & other) const {
    return n < other.n
        || (n == other.n && e < other.e);
  }

  bool operator == (Capture const & other) const {
    return n == other.n
        && e == other.e;
  }
};

constexpr Capture::State operator | (Capture::State a, Capture::State b) {
  return static_cast<Capture::State>(int(a) | b);
}

inline Capture::State & operator |= (Capture::State & a, Capture::State b) {
  a = static_cast<Capture::State>(a | b);
  return a;
}

constexpr Capture::State operator & (Capture::State a, Capture::State b) {
  return static_cast<Capture::State>(int(a) & b);
}

inline Capture::State & operator &= (Capture::State & a, Capture::State b) {
  a = static_cast<Capture::State>(a & b);
  return a;
}

constexpr Capture::State operator ~ (Capture::State a) {
  return static_cast<Capture::State>(~int(a) & (Capture::MAX - 1));
}

using Captures = std::vector<Capture>;

struct Range {
  enum State {
    None = 0,
    Normal = 1 << 0,
    Final = 1 << 1,
    Begin = 1 << 2,
    End = 1 << 3,
    Invalid = 1 << 4,
    INC_LAST_FLAG
  };
  State states;
  Captures capstates;
  Transitions transitions;

  bool operator == (Range const & other) const {
    return states == other.states
        && capstates == other.capstates
        && transitions == other.transitions;
  }
};

inline Range::State operator | (Range::State a, Range::State b) {
  return static_cast<Range::State>(int(a) | b);
}

inline Range::State & operator |= (Range::State & a, Range::State b) {
  a = static_cast<Range::State>(a | b);
  return a;
}

inline Range::State operator & (Range::State a, Range::State b) {
  return static_cast<Range::State>(int(a) & b);
}

inline Range::State & operator &= (Range::State & a, Range::State b) {
  a = static_cast<Range::State>(a & b);
  return a;
}

inline Range::State operator ~ (Range::State a) {
  return static_cast<Range::State>(~int(a) & (Range::INC_LAST_FLAG * 2 - 3));
}

struct Ranges : std::vector<Range>
{
  using std::vector<Range>::vector;
  using std::vector<Range>::operator=;
  std::vector<unsigned> capture_table;
};

template<class T>
bool operator != (T const & a, T const & b) {
  return !(a == b);
}

} }

#endif // REDFA_HPP
