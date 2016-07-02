#include "scan.hpp"
// #include "scan_intervals.hpp"
// #include "range_iterator.hpp"
// #include "trace.hpp"

#include <stdexcept>
// #include <algorithm>
#include <iostream>

#include <cassert>
#include <type_traits>

#define SEQ_ENUM(m)   \
  m(start)            \
  m(accu)             \
  m(quanti)           \
  m(quanti_bracket)   \
  m(backslash)        \
  m(interval)         \
  m(interval_Mm)      \
  m(interval_N)       \
  m(interval_Nn)      \
  m(bol)              \
  m(eol)              \
  m(is_bol)           \
  m(cap_open)         \
  m(cap_close)        \
  m(pipe)             \
  m(bracket_start)    \
  m(bracket_start2)   \
  m(bracket)          \
  m(bracket_interval) \
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

  constexpr Sc<
    accu
  + start
  + cap_open
  + pipe
  + bol
  + eol
  + quanti
  + quanti_bracket
  > bifurcation;
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

        CASE4(backslash, scan_escaped);

        CASE4(quanti, scan_quantifier);
        CASE4(quanti_bracket, scan_quantifier);

        CASE4(interval, scan_interval);
        CASE4(interval | interval_Mm, scan_interval);
        CASE4(interval | interval_Mm | interval_N, scan_interval);
        CASE4(interval | interval_Mm | interval_N | interval_Nn, scan_interval);
        CASE4(interval | interval_N, scan_interval);
        CASE4(interval | interval_N | interval_Nn, scan_interval);

        CASE4(bracket_start, scan_bracket);
        CASE4(bracket_start2, scan_bracket);
        CASE4(bracket, scan_bracket);
        CASE4(bracket_interval, scan_bracket);

        CASE4(cap_close, scan_capture_open);
        CASE4(cap_open, scan_capture_close);

        case U(error): throw parse_error("syntax error");
        default : throw parse_error("unimplemented [ " + to_string(e) + "]");
      }
    }
#undef U
#undef CASE3
#undef CASE4
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
      case '[': return s + bracket_start;
      case '|': return s + pipe;
      case '(': return s + cap_open;
      case ')': return s + cap_close;
      case '^': return s + bol;
      case '$': return s + eol;
      case '{':
      case '+':
      case '*':
      case '?': return error;
      case '.': ctx.single.e = {char_int{}, ~char_int{}}; return s + quanti;
      case '\\': return s + backslash;
      default : ctx.single.e = {c, c}; return s + quanti;
    }
  }

  template<S _s>
  S scan_quantifier(Sc<_s> s)
  {
    TT;
    switch (c) {
      case '+': return eval_quantifier_one_or_more(s);
      case '*': return eval_quantifier_zero_or_more(s);
      case '?': return eval_quantifier_optional(s);
      case '{': return s - quanti + interval;
      default: push_event(s); return scan_single(s - bifurcation);
    }
  }

  template<S _s>
  S scan_escaped(Sc<_s> s)
  {
    TT;
    switch (c) {
      //case 'd': return eval_quantifier_one_or_more(s);
      //case 'w': return eval_quantifier_zero_or_more(s);
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
  S eval_quantifier_one_or_more(Sc<_s> s)
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
  S eval_quantifier_zero_or_more(Sc<_s> s)
  {
    TT;
    if (s & accu) {

    }
    else {
//       rngs.back().push_back({e, rngs.size()-1});
    }
    return eval_quantifier_optional(s);
  }

  template<S _s>
  S eval_quantifier_optional(Sc<_s> s)
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
            ctx.interval.n += c - '0';
          }
          else {
            ctx.interval.n = c - '0';
          }
          return s + interval_Nn;
        }
        else {
          if (bool(s & interval_Mm)) {
            ctx.interval.m += c - '0';
          }
          else {
            ctx.interval.m = c - '0';
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
    if (s & bracket_start) {
      Sc<_s - bracket_start> new_s;
      ctx.bracket.is_reverse = ('^' == c);
      switch (c) {
        case '^': ctx.bracket.is_reverse = false; return new_s + bracket_start2;
        case ']': return eval_bracket_close(new_s);
        default :
          ctx.bracket.is_reverse = false;
          ctx.bracket.es.push_back({c, c});
          return new_s + bracket;
      }
    }

    if (s & bracket_start2) {
      Sc<_s - bracket_start2> new_s;
      switch (c) {
        case ']': return eval_bracket_close(new_s);
        default : ctx.bracket.es.push_back({c, c}); return new_s + bracket;
      }
    }

    if (s & bracket_interval) {
      Sc<_s - bracket_interval> new_s;
      switch (c) {
        case ']':
          ctx.bracket.es.push_back({'-', '-'});
          return eval_bracket_close(new_s);
        default :
          if (is_ascii_alnum(ctx.bracket.back().l)
           && is_ascii_alnum(c)
           && ctx.bracket.back().l <= c
          ) {
            ctx.bracket.back().r = c;
            return new_s + bracket_start2;
          }
          return error;
      }
    }

    if (s & bracket) {
      Sc<_s - bracket> new_s;
      switch (c) {
        case '-': return new_s + bracket_interval;
        case ']': return eval_bracket_close(new_s);
        default : ctx.bracket.es.push_back({c, c}); return s;
      }
    }

    return error;
  }

  template<S _s>
  S eval_bracket_close(Sc<_s> s)
  {
    TT;
    if (ctx.bracket.es.empty()) {
      return error;
    }
    if (ctx.bracket.is_reverse) {
      //reverse_transitions(ts, next_ts, state);
    }
    // rngs.emplace_back();
    return s + quanti_bracket;
  }

  template<S _s>
  S scan_or(Sc<_s> s)
  {
    TT;
    return s - pipe + start;
  }

  template<S _s>
  S scan_capture_open(Sc<_s> s)
  {
    TT;
    return s - cap_open;
  }

  template<S _s>
  S scan_capture_close(Sc<_s> s)
  {
    TT;
    return s - cap_close;
  }

  template<S _s>
  S scan_bol(Sc<_s> s)
  {
    TT;
    return s - bol;
  }

  template<S _s>
  S scan_eol(Sc<_s> s)
  {
    TT;
    return s - bifurcation - eol;
  }

  template<class If, class Else>
  decltype(auto) invoke_cond(std::false_type, If, Else f) { return f(); }

  template<class If, class Else>
  decltype(auto) invoke_cond(std::true_type, If f, Else) { return f(); }

  static bool is_ascii_alnum(char_int c)
  {
    return
      ('0' <= c && c <= '9') ||
      ('a' <= c && c <= 'z') ||
      ('A' <= c && c <= 'Z');
  }

  utf8_consumer consumer {nullptr};
  char_int c;
  struct {
    struct {
      Event e;
    } single;

    struct {
      unsigned m, n;
    } interval;

    struct {
      bool is_reverse;
      std::vector<Event> es;
      Event & back() { return es.back(); }
    } bracket;
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
