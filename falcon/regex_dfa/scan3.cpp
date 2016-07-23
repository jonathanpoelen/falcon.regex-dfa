#include "scan.hpp"
#include "print_automaton.hpp"
// #include "scan_intervals.hpp"
// #include "range_iterator.hpp"
// #include "trace.hpp"

#include <stdexcept>
#include <algorithm>

#include <bitset>
#include <type_traits>

#include <cassert>

#ifdef RE_TRACE
# include <iostream>
# define TRACE(expr)  std::cerr << expr << '\n'
#else
# define TRACE(expr)  void(1)
#endif


#define SEQ_ENUM(m)   \
  m(start)            \
  m(accu)             \
  m(reuse)            \
  m(subexpr)          \
  m(bol)              \
  m(eol)              \
  m(bracket)          \
  m(bracket_reversed)


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

constexpr S operator ~ (S a)
{ return static_cast<S>(~uint(a) & ((1u << States::NB) - 1u)); }

constexpr S operator & (S a, S b) { return static_cast<S>(uint(a) & uint(b)); }
constexpr S operator | (S a, S b) { return static_cast<S>(uint(a) | uint(b)); }
constexpr S operator + (S a, S b) { return static_cast<S>(uint(a) | uint(b)); }
constexpr S operator - (S a, S b) { return static_cast<S>(uint(a) & uint(~b)); }

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
    bol
  + eol
  + bracket
  + subexpr
  > previous_strong_state {};

  constexpr Sc<
    accu
  + start
  + previous_strong_state
  > bifurcation {};
}
// template<S s> constexpr std::integral_constant<bool, !s> operator ! (Sc<s>) { return {}; }
// constexpr std::true_type operator ! (std::false_type) { return {}; }
// constexpr std::false_type operator ! (std::true_type) { return {}; }

#ifdef RE_TRACE
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
#endif

#define TT TRACE(__PRETTY_FUNCTION__)

using parse_error = std::runtime_error;

// make new_scan && rlwrap ./new_scan |& sed -E 's/falcon::regex_dfa::scanner::basic_scanner:://g;s/with |void |\(\* (next|break)_act\)\(\) = |\(?falcon::regex_dfa::scanner::States::enum_t[) ]//g'

struct basic_scanner
{
  using act_t = void(basic_scanner::*)();
  using B = basic_scanner;

  void nil() {}

  [[noreturn]] void error() { throw parse_error{"error"}; }
  [[noreturn]] void error_end_of_string() { throw parse_error{"error end of string"}; }

  template<act_t next_act, act_t break_act = &B::error>
  void next()
  {
    TT;

    if ((c = consumer.bumpc())) {
      TRACE("- " << utf8_char(c) << " -");
      (this->*next_act)();
    }
    else {
      (this->*break_act)();
    }
  }

  void scan(const char * s)
  {
    TT;
    ctx.next_st = Range::None;
    consumer = utf8_consumer{s};
//     S e = start;

    next<&B::scan_single<start>, &B::final_start>();

    for (Range & r : ctx.rngs) {
      if (!r.states) {
        r.states = Range::Normal;
      }
    }

//     if (bool(e & escaped)) {
//       throw parse_error("error end of string");
//     }

//     if (bool(e & quanti)) {
//       //push_event(quanti, e);
//     }
  }

  void final_start()
  {
    TT;
    ctx.rngs.emplace_back();
    ctx.rngs.back().states = Range::Final;
  }

  template<S s>
  void scan_single()
  {
    TT;

    static_assert(!bool(s & (bracket | subexpr)), "invalid state");
    static_assert(!(bool(s & reuse) && bool(s & accu)), "invalid state");

    switch (c) {
      case '[': return eval_bracket<s-reuse>();
      case '|': return eval_pipe<s-reuse>();
      case '(': return eval_open_subexpr<s-reuse>();
      case ')': return eval_close_subexpr<s-reuse>();
      case '^': return eval_bol<s>();
      case '$': return eval_eol<s>();
      case '\\': return next<
        &B::scan_in_escape<s, &B::scan_single<s>>
      , &B::error_end_of_string>();
      case '{':
      case '+':
      case '*':
      case '?': return error();
      case '.':
        new_single<s>({char_int{}, ~char_int{}});
        return next<&B::scan_quanti<s>, &B::final_single<s&accu>>();
      default :
        new_single<s>({c, c});
        return next<&B::scan_quanti<s>, &B::final_single<s&accu>>();
    }
  }

  template<S s>
  void new_single(Event const & e)
  {
    TT;
    if (!bool(s & reuse)) {
      ctx.rngs.emplace_back();
      #ifdef RE_TRACE
      print_automaton(ctx.rngs);
      #endif
    }
    if (bool(s & (bol + eol))) {
      ctx.rngs.back().states = ctx.next_st;
      ctx.next_st = Range::None;
    }
    ctx.rngs.back().transitions.emplace_back(e, ctx.rngs.size());
  }

  template<S s>
  void final_single()
  {
    TT;
    if (bool(s & accu)) {
      auto & e = ctx.rngs.back().transitions.back().e;
      for (auto i : ctx.curr_rngs) {
        ctx.rngs[i].states |= Range::Final;
        ctx.rngs[i].transitions.emplace_back(e, ctx.rngs.size());
      }
    }
    ctx.rngs.emplace_back();
    ctx.rngs.back().states = Range::Final;
  }

  template<S s>
  void scan_quanti()
  {
    TT;
    switch (c) {
      case '*': return eval_closure0<s>();
      case '+': return eval_closure1<s-reuse>();
      case '?': return eval_opt<s-reuse>();
      case '{': return eval_brace<s-reuse>();
      default :
        if (bool(s & subexpr)) {
          //push_event<s - subexpr>();
        }
        if (bool(s & bracket)) {

        }
        else if (bool(s & accu)) {
          auto & e = ctx.rngs.back().transitions.back().e;
          for (auto i : ctx.curr_rngs) {
            ctx.rngs[i].transitions.emplace_back(e, ctx.rngs.size());
          }
        }
        // TODO stop_accu
        return scan_single<s - bifurcation - reuse>();
    }
  }

  template<S s>
  void eval_closure0()
  {
    TT;
    auto & e = ctx.rngs.back().transitions.back().e;
    if (bool(s & subexpr)) {
      // TODO
    }
    if (bool(s & accu)) {
      for (auto i : ctx.curr_rngs) {
        ctx.rngs[i].transitions.emplace_back(
          e,
          // TODO not eol
          bool(s & (bol | eol | reuse)) ? ctx.rngs.size() : ctx.rngs.size()-1
        );
      }
    }
    else {
      ctx.curr_rngs.clear();
    }

    // TODO not eol
    if (bool(s & (bol | eol | reuse))) {
      ctx.curr_rngs.emplace_back(ctx.rngs.size()-1);
      ctx.curr_rngs.emplace_back(ctx.rngs.size());
      ctx.rngs.emplace_back();
      ctx.rngs.back().transitions.emplace_back(e, ctx.rngs.size()-1);
    }
    else {
      ctx.curr_rngs.emplace_back(ctx.rngs.size()-1);
      ctx.rngs.back().transitions.emplace_back(e, ctx.rngs.size()-1);
    }

    return next<
      &B::scan_single<s + accu - previous_strong_state - reuse>
    , &B::final_closure0<s>
    >();
  }

  template<S s>
  void final_closure0()
  {
    TT;
    if (!bool(s & (bol | eol | reuse))) {
      Transitions & ts = ctx.rngs.back().transitions;
      ts[ts.size()-2].next = ts[ts.size()-1].next;
      ts.pop_back();
    }

    for (auto i : ctx.curr_rngs) {
      ctx.rngs[i].states |= Range::Final;
    }
    ctx.rngs.back().states |= Range::Final;
  }

  template<S s>
  void eval_closure1()
  {
    TT;
    auto & e = ctx.rngs.back().transitions.back().e;
    if (bool(s & subexpr)) {
      // TODO
    }
    if (bool(s & accu)) {
      for (auto i : ctx.curr_rngs) {
        ctx.rngs[i].transitions.emplace_back(e, ctx.rngs.size());
      }
    }
    ctx.rngs.emplace_back();
    ctx.rngs.back().transitions.emplace_back(e, ctx.rngs.size()-1);
    return next<
      &B::scan_single<s - bifurcation + reuse>
    , &B::final_closure1
    >();
  }

  void final_closure1()
  {
    TT;
    ctx.rngs.back().states |= Range::Final;
  }

  template<S s>
  void eval_opt()
  {
    TT;
    if (bool(s & subexpr)) {
      // TODO
    }
    if (bool(s & accu)) {
      auto & e = ctx.rngs.back().transitions.back().e;
      for (auto i : ctx.curr_rngs) {
        ctx.rngs[i].transitions.emplace_back(e, ctx.rngs.size());
      }
    }
    else {
      ctx.curr_rngs.clear();
    }
    ctx.curr_rngs.emplace_back(ctx.rngs.size()-1);
    return next<
      &B::scan_single<s + accu - previous_strong_state>
    , &B::final_opt>();
  }

  void final_opt()
  {
    TT;
    for (auto i : ctx.curr_rngs) {
      ctx.rngs[i].states |= Range::Final;
    }
    ctx.rngs.emplace_back();
    ctx.rngs.back().states |= Range::Final;
  }

  template<S s, act_t next_act>
  void scan_in_escape()
  {
    TT;
    switch (c) {
      //case 'd': return ;
      //case 'w': return ;
      default: push_event<s>(); return next<next_act>();
    }
  }

  template<S s>
  void push_event()
  {
    TT;
    if (bool(s & bracket)) {
    }
    else if (bool(s & accu)) {
      ctx.ts.emplace_back(ctx.single.e, ctx.rngs.size());
      for (auto u : ctx.curr_rngs) {
        Transitions & ts = ctx.rngs[u].transitions;
        ts.insert(ts.end(), ctx.ts.begin(), ctx.ts.end());
      }
    }
    else {
//       for (auto u : ctx.curr_rngs) {
//         Transitions & ts = ctx.rngs[u].transitions;
//         ts.emplace_back(ctx.single.e, ctx.rngs.size());
//       }
    }
  }

  template<S s>
  void events_to_rng()
  {
    if (bool(s & accu)) {

    }
    else {

    }
  }

  template<S s>
  void eval_brace()
  {
    TT;
    return next<
      &B::scan_in_brace<s, false>
    , &B::error_end_of_string>();
  }

  template<S s, bool is_empty_subsexpr>
  void scan_in_brace()
  {
    TT;
    switch (c) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (!is_empty_subsexpr) {
          ctx.interval.m = c - '0';
        }
        return next<&B::scan_in_brace_m<s, is_empty_subsexpr>>();
      case ',':
        if (!is_empty_subsexpr) {
          ctx.interval.n = 0;
        }
        return next<&B::scan_in_brace_x_n<s, is_empty_subsexpr, 0>>();
      default: return error();
    }
  }

  template<S s, bool is_empty_subsexpr>
  void scan_in_brace_m()
  {
    TT;
    switch (c) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (!is_empty_subsexpr) {
          ctx.interval.m *= 10;
          ctx.interval.m += c - '0';
        }
        return next<&B::scan_in_brace_m<s, is_empty_subsexpr>>();
      case ',':
        if (!is_empty_subsexpr) {
          ctx.interval.n = 0;
        }
        return next<&B::scan_in_brace_x_n<s, is_empty_subsexpr, 1>>();
      case '}': return eval_close_brace<is_empty_subsexpr ? s : s-accu>();
      default: return error();
    }
  }

  template<S s, bool is_empty_subsexpr, bool has_accu>
  void scan_in_brace_x_n()
  {
    TT;
    switch (c) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        if (!is_empty_subsexpr) {
          ctx.interval.n *= 10;
          ctx.interval.n += c - '0';
        }
        return next<&B::scan_in_brace_x_n<s, is_empty_subsexpr, has_accu>>();
      case '}': return eval_close_brace<
        is_empty_subsexpr ? s : has_accu ? s+accu : s-accu
      >();
      default: return error();
    }
  }

  template<S s>
  void eval_close_brace()
  {
    return next<&B::scan_single<s - previous_strong_state>>();
  }

  template<S s>
  void eval_bracket()
  {
    TT;
    return next<
      &B::scan_in_bracket<s + bracket>
    , &B::error_end_of_string>();
  }

  template<S s>
  void scan_in_bracket()
  {
    TT;
    ctx.bracket.is_reverse = false;
    switch (c) {
      case '^':
        ctx.bracket.is_reverse = true;
        return next<
          &B::scan_in_bracket_first<s+bracket_reversed>
        , &B::error_end_of_string>();
      default :
        return scan_in_bracket_first<s>();
    }
  }

  template<S s>
  void scan_in_bracket_first()
  {
    TT;
    switch (c) {
      case ']':
        if (bool(s & bracket_reversed)) {
          return error();
        }
        return eval_empty_bracket_close<s>();
      case '-':
        ctx.bracket.push_back(c);
        return next<&B::scan_in_bracket_a<s>>();
      case '\\':
        return next<
          &B::scan_in_escape<s, &B::scan_in_bracket_a<s>>
        , &B::error_end_of_string>();
      default :
        ctx.bracket.push_back(c);
        return next<&B::scan_in_bracket_a<s>, &B::error>();
    }
  }

  template<S s>
  void scan_in_bracket_a()
  {
    TT;
    switch (c) {
      case ']': return eval_bracket_close<s>();
      case '\\':
        return next<
          &B::scan_in_escape<s, &B::scan_in_bracket_b<s>>
        , &B::error_end_of_string>();
      default :
        ctx.bracket.push_back(c);
        return next<&B::scan_in_bracket_b<s>, &B::error>();
    }
  }

  template<S s>
  void scan_in_bracket_b()
  {
    TT;
    switch (c) {
      case '-': return scan_in_bracket_r<s>();
      case ']': return eval_bracket_close<s>();
      case '\\':
        return next<
          &B::scan_in_escape<s, &B::scan_in_bracket_a<s>>
        , &B::error_end_of_string>();
      default :
        ctx.bracket.push_back(c);
        return next<&B::scan_in_bracket_a<s>, &B::error>();
    }
  }

  template<S s>
  void scan_in_bracket_r()
  {
    TT;
    switch (c) {
      case ']': return eval_bracket_close<s>();
      case '\\':
        return next<
          &B::scan_in_escape<s, &B::scan_in_bracket_a<s>>
        , &B::error_end_of_string>();
      default :
        ctx.bracket.back().r = c;
        return next<&B::scan_in_bracket_a<s>, &B::error>();
    }
  }

  template<S s>
  void eval_bracket_close()
  {
    if (bool(s & bracket_reversed)) {
      reverse_bracket();
    }

    return eval_empty_bracket_close<s>();
  }

  template<S s>
  void eval_empty_bracket_close()
  {
    return next<
      &B::scan_quanti<s - bracket_reversed>
    , &B::push_event<s - bracket_reversed>>();
  }


  template<S s>
  void eval_open_subexpr()
  {
    TT;
    return next<&B::scan_maybe_ext_subexpr<s>, &B::error_end_of_string>();
  }

  template<S s>
  void scan_maybe_ext_subexpr()
  {
    TT;
    switch (c) {
      case ')': return next<&B::scan_quanti_empty_subexpr<s>>();
      case '?': return next<&B::scan_ext_subexpr<s>, &B::error_end_of_string>();
      default : return eval_subexpr<s>();
    }
  }

  template<S s>
  void scan_ext_subexpr()
  {
    TT;
    switch (c) {
      case '!': return eval_subexpr<s>();
      default : return error();
    }
  }

  template<S s>
  void scan_quanti_empty_subexpr()
  {
    TT;
    switch (c) {
      case '{': return next<&B::scan_in_brace<s, 1>, &B::error_end_of_string>();
      case '*':
      case '+':
      case '?': return next<&B::scan_single<s>>();
      default : return scan_single<s>();
    }
  }

  template<S s>
  void eval_subexpr()
  {
    TT;
    ++ctx.subexpr.depth;
    if (bool(s & accu)) {
      if (ctx.subexpr.depth > Ctx::Subexpr::max_depth) {
        return error();
      }
      ctx.subexpr.is_accu_flags.set(ctx.subexpr.depth);
      // TODO
    }
    return next<&B::scan_single<start>>();
  }

  template<S s>
  void eval_close_subexpr()
  {
    TT;
    if (!ctx.subexpr.depth) {
      return error();
    }

    {
      if (ctx.subexpr.is_accu_flags.test(ctx.subexpr.depth)) {
        ctx.subexpr.is_accu_flags.reset(ctx.subexpr.depth);
        // TODO
      }
    }

    ctx.subexpr.depth--;
    return next<&B::scan_quanti<s + subexpr>>();
  }

  template<S s>
  void eval_pipe()
  {
    TT;
    // TODO
    return next<&B::scan_single<s>>();
  }

  template<S s>
  void eval_bol()
  {
    TT;
    if (!bool(s & start)) {
      // TODO
    }

    ctx.next_st |= Range::Bol;
    return next<&B::scan_single<s + bol>>();
  }

  template<S s>
  void eval_eol()
  {
    TT;
    ctx.next_st |= Range::Eol;
    return next<&B::scan_single<s + eol>>();
  }

  static bool is_ascii_alnum(char_int c)
  {
    return
      ('0' <= c && c <= '9') ||
      ('a' <= c && c <= 'z') ||
      ('A' <= c && c <= 'Z');
  }

  void reverse_bracket()
  {
    auto & ts = ctx.bracket.es;
    std::sort(ts.begin(), ts.end(), [](Event & e1, Event & e2) {
      return e1.l < e2.l;
    });
    auto pred = [](Event & e1, Event & e2) {
      return e2.l <= e1.r || e1.r + 1 == e2.l;
    };
    auto first = std::adjacent_find(ts.begin(), ts.end(), pred);
    auto last = ts.end();
    if (first != last) {
      auto result = first;
      result->r = std::max(result->r, first->r);
      while (++first != last) {
        if (pred(*result, *first)) {
          result->r = std::max(result->r, first->r);
        }
        else {
          ++result;
          *result = *first;
        }
      }
      ts.resize(result - ts.begin() + 1);
    }

    first = ts.begin();
    last = ts.end();

    if (ts.front().l) {
      auto c1 = first->r;
      first->r = first->l-1;
      first->l = 0;
      for (; ++first !=last; ) {
        auto c2 = first->l;
        first->l = c1+1;
        c1 = first->r;
        first->r = c2-1;
      }
      if (c1 != ~char_int{}) {
        ts.push_back({++c1, ~char_int{}});
      }
    }
    else {
      --last;
      for (; first != last; ++first) {
        first->l = first->r + 1;
        first->r = (first+1)->l - 1;
      }
      if (first->r != ~char_int{}) {
        first->r = ~char_int{};
      }
      else {
        ts.pop_back();
      }
    }
  }

  utf8_consumer consumer {nullptr};
  char_int c;
  struct Ctx {
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
      void push_back(char_int c) { es.push_back({c, c}); }
    } bracket;

    struct Subexpr {
      static constexpr std::size_t max_depth = 64;
      using bitset_t = std::bitset<max_depth>;

      unsigned depth = 0;
      bitset_t is_accu_flags = 0;
    } subexpr;

    struct Pipe {

    } pipe;

    Ranges rngs;
    std::vector<unsigned> curr_rngs;
    Range::State next_st;
    Transitions ts;
  } ctx;
};

}

Ranges scan(const char * s)
{
  scanner::basic_scanner scanner;
//   scanner.prepare();
  scanner.scan(s);
//   return scanner.final();
  return std::move(scanner.ctx.rngs);
}

}
}
