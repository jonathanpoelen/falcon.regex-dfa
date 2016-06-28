#include "scan.hpp"
// #include "scan_intervals.hpp"
// #include "range_iterator.hpp"
// #include "trace.hpp"

#include <stdexcept>
// #include <algorithm>
#include <iostream>

#include <cassert>
#include <type_traits>

#define SEQ_ENUM(m) \
  m(start)          \
  m(accu)           \
  m(quanti)         \
  m(backslash)      \
  m(interval)       \
  m(interval_Mm)    \
  m(interval_N)     \
  m(interval_Nn)    \
  m(bol)            \
  m(eol)            \
  m(open_cap)       \
  m(close_cap)      \
  m(cap)            \
  m(pipe)           \
  m(error)

namespace falcon {
namespace regex_dfa {
namespace scanner {

struct States {
  enum flag_e {
#define DEFINED_TOKEN(e) _##e,
    SEQ_ENUM(DEFINED_TOKEN)
#undef DEFINED_TOKEN
    NB
  };

  enum class enum_t : unsigned {
    none,
#define DEFINED_FLAG(e) e = 1 << _##e,
    SEQ_ENUM(DEFINED_FLAG)
#undef DEFINED_FLAG
  };
};

using S = States::enum_t;

constexpr S operator & (S a, S b) { return static_cast<S>(uint(a) & uint(b)); }
constexpr S operator | (S a, S b) { return static_cast<S>(uint(a) | uint(b)); }
constexpr S operator + (S a, S b) { return static_cast<S>(uint(a) | uint(b)); }
constexpr S operator - (S a, S b) { return static_cast<S>(uint(a) & ~uint(b)); }

template<S s> struct Sc : std::integral_constant<S, s>
{
  explicit constexpr operator bool () const { return bool(s); }
  constexpr operator S () const { return s; }
};

template<S s1, S s2> constexpr Sc<s1+s2> operator + (Sc<s1>, Sc<s2>) { return {}; }
template<S s1, S s2> constexpr Sc<s1-s2> operator - (Sc<s1>, Sc<s2>) { return {}; }
template<S s1, S s2> constexpr Sc<s1&s2> operator & (Sc<s1>, Sc<s2>) { return {}; }
template<S s1, S s2> constexpr Sc<s1|s2> operator | (Sc<s1>, Sc<s2>) { return {}; }

namespace {
#define DEFINED_VALUE_EXPR(e) constexpr Sc<S::e> e {};
    DEFINED_VALUE_EXPR(none)
    SEQ_ENUM(DEFINED_VALUE_EXPR)
#undef DEFINED_VALUE_EXPR

  constexpr Sc<accu + start + open_cap + pipe + bol + eol + quanti> bifurcation;
}
// template<S s> constexpr std::integral_constant<bool, !s> operator ! (Sc<s>) { return {}; }
// constexpr std::true_type operator ! (std::false_type) { return {}; }
// constexpr std::false_type operator ! (std::true_type) { return {}; }

std::string to_string(S s)
{
  std::string str;
  if (!bool(s)) {
    str = "none ";
  }
  else {
#define TO_STR(e) if (bool(s & S::e)) { str += #e " "; }
    SEQ_ENUM(TO_STR)
#undef TO_STR
  }
  return str;
}

std::ostream & operator<<(std::ostream & out, S s)
{
  if (!bool(s)) {
    return out << "none ";
  }
#define WRITE_IF(e) if (bool(s & S::e)) { out << #e " "; }
    SEQ_ENUM(WRITE_IF)
#undef WRITE_IF
  return out;
}

#define TT std::cerr << __PRETTY_FUNCTION__ << '\n'
// #define TT void(1)

using parse_error = std::runtime_error;

// make new_scan && rlwrap ./new_scan |& sed -E 's/(.*)falcon::regex_dfa::scanner::S falcon::regex_dfa::scanner::basic_scanner::([^(]+).*enum_t\)([^u]+)u.*/\1\2(<\3>)/g'

struct basic_scanner
{

  void scan(const char * s)
  {
    TT;
    consumer = utf8_consumer{s};
    S e = start;

#define U(s) static_cast<unsigned>(static_cast<S>(s))
#define CASE(evt, func) case U(evt): e = func(evt); break
#ifdef IN_IDE_PARSER
#define CASE3 CASE
#define CASE4 CASE
#else
#define CASE3(evt, func)   \
  CASE(start | evt, func); \
  CASE(none  | evt, func); \
  CASE(accu  | evt, func);
#define CASE4(evt, func) \
  CASE3(evt, func);      \
  CASE(start | accu | evt, func)
#endif
    while ((c = consumer.bumpc())) {
      std::cerr << utf8_char(c) << " [ " << e << "] > ";
      switch (U(e)) {
        CASE4(none, scan_single);

        CASE3(backslash, scan_escaped);

        CASE4(quanti, scan_quantifier);

        CASE3(interval, scan_interval);
        CASE3(interval | interval_Mm, scan_interval);
        CASE3(interval | interval_Mm | interval_N, scan_interval);
        CASE3(interval | interval_Mm | interval_N | interval_Nn, scan_interval);
        CASE3(interval | interval_N, scan_interval);
        CASE3(interval | interval_N | interval_Nn, scan_interval);

        case U(error): throw parse_error("syntax error");
        default : throw parse_error("unimplemented [ " + to_string(e) + "]");
      }
    }
#undef U
#undef CASE3
#undef CASE
    if (bool(e & backslash)) {
      throw parse_error("error end of string");
    }

    if (bool(e & quanti)) {
      //push_event(quanti, e);
    }
  }

  template<S _s>
  S scan_single(Sc<_s> s)
  {
    TT;
    switch (c) {
      case '[': return scan_bracket(s);
      case '|': return scan_or(s);
      case '(': return scan_open(s);
      case ')': return scan_close(s);
      case '^': return scan_bol(s);
      case '$': return scan_eol(s);
      case '{':
      case '+':
      case '*':
      case '?': return scan_error();
      case '.': ctx.e = {char_int{}, ~char_int{}}; return s + quanti;
      case '\\': return s + backslash;
      default : ctx.e = {c, c}; return s + quanti;
    }
  }

  template<S _s>
  S scan_quantifier(Sc<_s> s)
  {
    TT;
    switch (c) {
      case '+': return scan_quantifier_one_or_more(s);
      case '*': return scan_quantifier_zero_or_more(s);
      case '?': return scan_quantifier_optional(s);
      case '{': return s - quanti + interval;
      default: push_event(s); return scan_single(s - bifurcation);
    }
  }

  template<S _s>
  S scan_escaped(Sc<_s> s)
  {
    TT;
    switch (c) {
      //case 'd': return scan_quantifier_one_or_more(s);
      //case 'w': return scan_quantifier_zero_or_more(s);
      default: push_event(s); return s - backslash + quanti;
    }
  }

  template<S _s>
  S push_event(Sc<_s> s)
  {
    TT;
    return s - bifurcation;
  }

  template<S _s>
  S scan_quantifier_one_or_more(Sc<_s> s)
  {
    TT;
    if (s & accu) {

    }
    else {
//       rngs.back().emplace_back(e, rngs.size()-1);
//       rngs.back().emplace_back(e, rngs.size());
    }
//     rngs.emplace_back();
    return s - bifurcation;
  }

  template<S _s>
  S scan_quantifier_zero_or_more(Sc<_s> s)
  {
    TT;
    if (s & accu) {

    }
    else {
//       rngs.back().push_back({e, rngs.size()-1});
    }
    return scan_quantifier_optional(s);
  }

  template<S _s>
  S scan_quantifier_optional(Sc<_s> s)
  {
    TT;
    if (s & accu) {

    }
    else {
//       rngs.emplace_back();
//       irngs.clear();
//       irngs.emplace_back(rngs.size()-2);
//       irngs.emplace_back(rngs.size()-1);
    }
    return s + accu - quanti;
  }

  template<S _s>
  S scan_interval(Sc<_s> s)
  {
    TT;
    // {interval_Mm ,interval_N interval_Nn}
    // {interval_Mm}
    // {interval_Mm ,interval_N}
    // {,interval_N interval_Nn}
    switch (c) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (bool(s & (interval_N | interval_Nn))) {
          if (bool(s & interval_Nn)) {
            ctx.n += c - '0';
          }
          else {
            ctx.n = c - '0';
          }
          return s + interval_Nn;
        }
        else {
          if (bool(s & interval_Mm)) {
            ctx.m += c - '0';
          }
          else {
            ctx.m = c - '0';
          }
          return s + interval_Mm;
        }
        return s + interval_N;
      case ',': return (s & interval_N) ? error() : s + interval_N;
      case '}': return (s & (interval_Mm | interval_Nn))
        ? s - interval_Mm - interval_N - interval_Nn
        : error();
      default: return error;
    }
  }

  template<S _s>
  S scan_bracket(Sc<_s> s)
  {
    TT;
    return s;
  }

  template<S _s>
  S scan_or(Sc<_s> s)
  {
    TT;
    return s + pipe;
  }

  template<S _s>
  S scan_open(Sc<_s> s)
  {
    TT;
    return s + open_cap + cap;
  }

  template<S _s>
  S scan_close(Sc<_s> s)
  {
    TT;
    return s + close_cap;
  }

  template<S _s>
  S scan_bol(Sc<_s> s)
  {
    TT;
    return s + bol;
  }

  template<S _s>
  S scan_eol(Sc<_s> s)
  {
    TT;
    return s + eol;
  }

  S scan_error()
  {
    TT;
    return error;
  }

  template<class If, class Else>
  decltype(auto) invoke_cond(std::false_type, If, Else f) { return f(); }

  template<class If, class Else>
  decltype(auto) invoke_cond(std::true_type, If f, Else) { return f(); }

  utf8_consumer consumer {nullptr};
  char_int c;
  struct {
    Event e;
    unsigned m, n;
  } ctx;
};

}

Ranges scan(const char * s)
{
  scanner::basic_scanner scanner;
//   scanner.prepare();
  scanner.scan(s);
//   return scanner.final();
  return Ranges{};
}

}
}
