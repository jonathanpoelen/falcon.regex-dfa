#include "scan.hpp"
#include "scan_intervals.hpp"

#include <stdexcept>
#include <algorithm>

#include <cassert>
// #include <iostream>
// #include "print_automaton.hpp"

namespace falcon { 
namespace regex_dfa {
namespace {

using index_type = unsigned;
using size_t = unsigned;

struct Stack {
  std::vector<index_type> iends;
  std::vector<index_type> irngs;
  falcon::regex_dfa::Range::State states;
  index_type ipipe;
  size_t count_pipe;
};

class CapStack
{
  using cap_flags_t = unsigned long long;
  static constexpr cap_flags_t max_cap_level = sizeof(cap_flags_t) * __CHAR_BIT__;
  unsigned nb_cap = 0;
  cap_flags_t cap_flags = 0;
  unsigned stack_capture[max_cap_level];
  unsigned * pcapture = stack_capture;
  falcon::regex_dfa::Captures capstates;
  std::vector<falcon::regex_dfa::Captures> capspipe;

public:
  std::vector<unsigned> capture_table;

  falcon::regex_dfa::Captures const & captures() const {
    return capstates;
  }

  bool is_full(std::size_t level) const {
    return level == max_cap_level;
  }

  void mark(std::size_t level) {
    cap_flags |= mask(level);
    *pcapture = unsigned(capture_table.size());
    capture_table.push_back(*pcapture);
    capstates.push_back({nb_cap, falcon::regex_dfa::Capture::Active | falcon::regex_dfa::Capture::Open});
    ++pcapture;
    ++nb_cap;
    capspipe.push_back(capstates);
  }

  bool is_marked(std::size_t level) const {
    return cap_flags & mask(level);
  }

  void alternation() {
    if (!capspipe.empty()) {
      capstates = capspipe.back();
    }
  }

  void remove_open() {
    if (!captures().empty()) {
      //while (!(capstates.back().e &= ~Capture::Open)) {
      //  capstates.pop_back();
      //}
      auto first = capstates.begin();
      auto last = capstates.end();
      for (; first != last; ++first) {
        first->e &= ~falcon::regex_dfa::Capture::Open;
      }
    }
  }

  unsigned unmark(std::size_t level) {
    capstates.back().e &= ~falcon::regex_dfa::Capture::Open;
    --pcapture;
    capture_table.push_back(capture_table[*pcapture]);
    capture_table[*pcapture] = unsigned(capture_table.size())-1u;
    cap_flags &= ~mask(level);
    auto n = capstates.back().n;
    capstates.pop_back();
    capspipe.pop_back();
    return n;
  }

private:
  static std::size_t mask(std::size_t level) {
    return std::size_t{1} << level;
  }
};

struct Pipe {
  index_type ipipe;
  index_type irngs_size;
  bool is_empty;
};

template<class It>
struct range_iterator
{
  It first, last;
  It begin() const { return first; }
  It end() const { return last; }
  std::size_t size() const { return last - first; }
};

using std::swap;

/// @{
/// utility
template<class C1, class C2, class C3, class It, class Cmp = std::less<>>
static void union_safe(C1 & c, C2 const & c2, C3 & tmp, It tmp_begin, Cmp cmp = Cmp{}) {
  tmp.erase(
    std::set_union(
      c.begin(), c.end(),
      c2.begin(), c2.end(),
      tmp_begin, cmp
    ),
    tmp.end()
  );
}

template<class C, class C2, class Cmp = std::less<>>
static void set_union(C & c, C2 const & c2, C & tmp, Cmp cmp = Cmp{}) {
  if (!c2.size()) {
    return ;
  }
  tmp.resize(c.size() + c2.size());
  union_safe(c, c2, tmp, tmp.begin(), cmp);
  swap(c, tmp);
}
/// @}

struct basic_scanner
{
  std::vector<Stack> stack;
  std::vector<Pipe> pipe_stack;
  CapStack cap_stack;
 
  Ranges rngs;
  std::vector<index_type> irngs;
  std::vector<index_type> iends;
  index_type ipipe;

  Range::State states;
  
  Transitions ts;

  /// @{
  /// garbage 
  Captures tmp_capstates;
  std::vector<Range> new_rng;
  decltype(irngs) cp_irngs;
  decltype(cp_irngs) tmp_irng;
  /// @}
  
  utf8_consumer consumer {nullptr};
  char_int c;
  
  void prepare() {
    stack.reserve(8);
    rngs.reserve(8);
    irngs.reserve(8);
    stack.push_back({{}, {}, Range::Normal, 0, 0});
    rngs.push_back({Range::Normal, {}, {}});
    irngs.push_back(0);
    ipipe = 0;
    states = Range::Normal;
  }
  
  void scan(const char * s)
  {
    consumer = utf8_consumer{s};
    c = consumer.bumpc();

    check_no_repetition();
    while (c) {
      enum { Transition, Character };

      if ([&]{
        switch (c) {
          case '[': scan_bracket(); return Character;
          case '|': scan_or(); return Transition;
          case '(': scan_open(); return Transition;
          case ')': scan_close(); return Transition;
          case '^': scan_begin(); return Transition;
          case '$': scan_end(); return Transition;
          default: scan_normal(); return Character;
        }
      }()) {
         switch (c) {
           case '+': scan_single_one_or_more(); break;
           case '*': scan_single_zero_or_more(); break;
           case '?': scan_single_optional(); break;
           case '{': scan_single_interval(); break;
           default: add_single_range();
        }
      }
      else {
        check_no_repetition();
      }
    }
  }
  
  Ranges final() 
  {
    if (stack.size() > 1) {
      throw std::runtime_error("unmatched )");
    }

    if (group_close() && !(rngs[0].states & Range::End)) {
      rngs[0].states |= Range::Final;
      rngs[0].states &= ~Range::Normal;
    }
    //pipe_stack.clear();
    //irngs.insert(irngs.end(), iends.begin(), iends.end());

    auto re_states = [this](auto & iarr){
      for (auto i : iarr) {
        if (!(rngs[i].states & Range::End)) {
          rngs[i].states |= Range::Final | states;
          rngs[i].states &= ~Range::Normal;
        }
      }
    };

    re_states(iends);
    re_states(irngs);

    rngs.capture_table = std::move(cap_stack.capture_table);
    return std::move(rngs);
  }
  
  /// \return  true if count_rngs()-1 in ipipes, otherwhise false
  bool group_close() 
  {
    //if (ipipe == stack.back().ipipe) {
    //  return true;
    //}
    unsigned new_i = count_rngs()-1u;
    bool has_ipipe = ipipe == new_i;
    ipipe = stack.back().ipipe;
    
    unsigned const count_pipe = unsigned(pipe_stack.size() - stack.back().count_pipe);
    assert(count_pipe <= pipe_stack.size());
    auto first = pipe_stack.end() - count_pipe;
    auto last = pipe_stack.end();
    auto p = iends.begin();
    auto cp = p;
    auto cur_ipipe = ipipe;
    
    for (; first != last; ++first) {
      auto old = first->ipipe;

      assert(first->irngs_size <= iends.size() - (iends.begin() - p));
      assert(first->irngs_size);
      auto e = p + first->irngs_size;
      has_ipipe = has_ipipe || p[0] == cur_ipipe;
      if (!first->is_empty) {
        auto last = e[-1] == old ? e-1 : e;
        if (p == cp) {
          cp = last;
        }
        else {
          cp = std::copy(p, last, cp);
        }
        p = e;
      }
      
      auto i = cur_ipipe;
      auto ie = i + first->irngs_size;
      assert(ie <= rngs.size());
      for (; i != ie; ++i) {
        for (Transition & t : rngs[i].transitions) {
          if (t.next == old) {
            t.next = new_i;
          }
        }
      }
      cur_ipipe = old;
    }
    
    iends.erase(cp, iends.end());
  
    first = pipe_stack.end() - count_pipe;
    Transitions & ts_dest = rngs[ipipe].transitions;
    for (; first != last; ++first) {
      if (ipipe != first->ipipe) {
        Transitions & ts_src = rngs[first->ipipe].transitions;
        ts_dest.insert(ts_dest.end(), ts_src.begin(), ts_src.end());
      }
    }

    return has_ipipe;
  }
  
  void check_no_repetition() {
    switch (c) {
      case '?': case '+': case '*': case '{':
      throw std::runtime_error("syntaxe error ? + * {");
    }
  }
  
  std::size_t cap_level() {
    return stack.size() - 1u; 
  }

  template<class Bool>
  Range & new_range(Bool remove_open_cap) {
    rngs.push_back({states, cap_stack.captures(), {}});
    if (remove_open_cap) {
      cap_stack.remove_open();
    }
    states = Range::Normal;
    return rngs.back();
  }

  void set_transitions() {
    for (auto && i : irngs) {
      auto & rng_ts = rngs[i].transitions;
      rng_ts.insert(rng_ts.end(), ts.begin(), ts.end());
    }
  }

  unsigned count_rngs() {
    return static_cast<unsigned>(rngs.size()); 
  }

  void renext_transitions() {
    for (auto && t : ts) {
      t.next = count_rngs();
    }
  }

  enum class MultiDup { Repeat, Conditional, RepeatAndConditional, RepeatAndMore };

  template<class EState>
  void repeat_multi_dup(unsigned long m, unsigned long n, EState estate)
  {
    new_rng.assign(rngs.begin() + ipipe, rngs.end());
    auto count_rng_added = unsigned(new_rng.size() - 1u);

    unsigned const skip_ipipe = irngs[0] == ipipe ? 1u : 0u;

    cp_irngs.clear();
    if (estate == MultiDup::Conditional) {
      if (!skip_ipipe) {
        cp_irngs.push_back(ipipe);
      }
      cp_irngs.insert(cp_irngs.end(), irngs.begin(), irngs.end());
    }

    auto update_transition_indexes = [&] {
      for (auto & r : new_rng) {
        for (auto && t : r.transitions) {
          t.next += count_rng_added;
        }
      };
    };
    
    std::vector<unsigned> extended_irng;
    if (skip_ipipe) {
      for (auto & t : rngs[0].transitions) {
        if (std::binary_search(irngs.begin(), irngs.end(), t.next)) {
          extended_irng.push_back(static_cast<unsigned>(t.next));
        }
      }
    }
    
    auto insert_transitions = [&](Transitions const & transitions_added){
      for (auto & i : irngs) {
        Range & r = rngs[i];
        r.states |= states;
        set_union(r.capstates, cap_stack.captures(), tmp_capstates);
        r.transitions.insert(r.transitions.end(), transitions_added.begin(), transitions_added.end());
      }
    };

    using range_t = range_iterator<decltype(irngs.begin())>;

    rngs.reserve((new_rng.size() - 1) * (m - 1));

    auto update_rngs = [&] {
      update_transition_indexes();
      rngs.insert(rngs.end(), new_rng.begin() + 1, new_rng.end());
      insert_transitions(new_rng[0].transitions);
      for (auto && i : range_t{irngs.begin() + skip_ipipe, irngs.end()}) {
        i += count_rng_added;
      }
      set_union(irngs, extended_irng, tmp_irng);
    };
    
    while (--m) {
      if (estate == MultiDup::Conditional) {
        set_union(irngs, cp_irngs, tmp_irng);
      }
      update_rngs();
    }

    if (estate == MultiDup::RepeatAndMore) {
      ts = new_rng[0].transitions;
    }
    
    auto insert_states = [&](){
      for (auto & i : irngs) {
        Range & r = rngs[i];
        r.states |= states;
        set_union(r.capstates, cap_stack.captures(), tmp_capstates);
      }
    };

    if (estate != MultiDup::RepeatAndConditional) {
      insert_states();
      return ;
    }

    if (!n) {
      insert_states();
      return ;
    }

    cp_irngs.assign(irngs.begin() + skip_ipipe, irngs.end());
    
    while (n--) {
      range_t r{irngs.begin() + skip_ipipe, irngs.end()};
      set_union(cp_irngs, r, tmp_irng);
      update_rngs();
    }
    set_union(irngs, cp_irngs, tmp_irng);
    insert_states();
  }

  void scan_bracket() {
    ts.clear();
    scan_intervals(consumer, ts, count_rngs());
    c = consumer.bumpc();
  }
  
  void scan_or() {
    bool const is_empty = (count_rngs()-1 == ipipe);
    ipipe = count_rngs()-1;
    if (!rngs[ipipe].transitions.empty()) {
      Range & rng_base = rngs[stack.back().ipipe];
      states = rng_base.states;
      // TODO cap_stack
      rngs.push_back({{}, {}, {}});
      ipipe = count_rngs()-1;
    }
    if (is_empty && !iends.empty() && iends.back() == irngs.front()) {
      pipe_stack.back().is_empty = true;
      pipe_stack.push_back({ipipe, unsigned(irngs.size()), unsigned(irngs.size())});
    }
    else {
      pipe_stack.push_back({ipipe, unsigned(irngs.size()), false});
      if (iends.empty()) {
        swap(iends, irngs);
      }
      else {
        iends.insert(iends.end(), irngs.begin(), irngs.end());
      }
    }
    irngs.clear();
    irngs.push_back(ipipe);
    c = consumer.bumpc();
    states = stack.back().states;
    cap_stack.alternation();
  }

  void scan_multi_one_or_more() {
    ts = rngs[stack.back().ipipe].transitions;
    set_transitions();
    c = consumer.bumpc();
  }

  void scan_multi_zero_or_more() {
    ts = rngs[stack.back().ipipe].transitions;
    set_transitions();
    c = consumer.bumpc();
  }

  void scan_multi_optional() {
    c = consumer.bumpc();
  }

  bool scan_multi_interval() {
    auto start = consumer.str();
    char * end = 0;

    auto exec_state = [&, this](unsigned long m, auto estate, auto check) {
      unsigned long n = 0;

      if (estate != MultiDup::Repeat) {
        n = strtoul(start, &end, 10);

        if (end == start) {
          throw std::runtime_error("invalid syntaxe {,}");
        }
        if (!n) {
          throw std::runtime_error("numbers is 0 in {}");
        }
        n = check(n);

        if (estate == MultiDup::Conditional) {
          if (!n) {
            return ;
          }
          m = n;
        }
      }

      this->repeat_multi_dup(m, n, estate);
    };

    auto repeat_element = [&](unsigned long n){
      repeat_multi_dup(n, 0, std::integral_constant<MultiDup, MultiDup::Repeat>{});
    };

    auto repeat_conditional = [&](auto check) {
      exec_state(
        0u,
        std::integral_constant<MultiDup, MultiDup::Conditional>{},
        check
      );
    };

    auto repeat_element_and_conditional = [&](unsigned long m, auto check) {
      exec_state(
        m,
        std::integral_constant<MultiDup, MultiDup::RepeatAndConditional>{},
        check
      );
    };
    
    bool merge_irng = false;

    // {,n}
    if (*start == ',') {
      ++start;
      repeat_conditional([](auto n) {return n;});
      merge_irng = true;
    }
    // {m...}
    else {
      unsigned long m = strtoul(start, &end, 10);

      if (end == start) {
        throw std::runtime_error("invalid syntaxe in {}");
      }
      if (0u == m) {
        throw std::runtime_error("numbers is 0 in {}");
      }

      if (*end == ',') {
        // {m,}
        if (*++end == '}') {
          repeat_multi_dup(m, 0, std::integral_constant<MultiDup, MultiDup::RepeatAndMore>{});
          set_transitions();
        }
        // {m,n}
        else {
          start = end;
          repeat_element_and_conditional(m, [&](auto n) {
            if (m > n) {
              throw std::runtime_error("numbers out of order in {} quantifier");
            }
            return n-m;
          });
        }
      }
      // {m}
      else {
        repeat_element(m);
      }
    }
    if (*end != '}') {
      throw std::runtime_error("unmatched }");
    }
    consumer.str(end + 1);
    c = consumer.bumpc();
    return merge_irng;
  }

  void scan_open() {
    if (cap_stack.is_full(cap_level())) {
      throw std::runtime_error("capture overflow");
    }
    if ('?' == (c = consumer.bumpc())) {
      switch (c = consumer.bumpc()) {
        case '!':
          c = consumer.bumpc();
          break;
        case ':':
          throw std::runtime_error("not implemented: (?:");
          break;
        case '=':
          throw std::runtime_error("not implemented: (?=");
          break;
        default:
          throw std::runtime_error("unrecognized (?x");
          break;
      }
    }
    else {
      cap_stack.mark(cap_level());
    }
    ipipe = count_rngs()-1;
    stack.push_back({
      std::move(iends),
      std::move(irngs),
      states,
      ipipe,
      unsigned(pipe_stack.size())
    });
    iends.clear();
    irngs.clear();
    irngs.push_back(ipipe);
  }

  void scan_close() {
    if (!cap_level()) {
      throw std::runtime_error("unmatched )");
    }
    
    bool const has_ipipe = group_close();
    pipe_stack.resize(stack.back().count_pipe);
    if (!iends.empty()) {
      iends.insert(iends.end(), irngs.begin(), irngs.end());
      swap(iends, irngs);
    }
    
    auto copy_first = [this]{
      auto & irngs_prev = stack.back().irngs;
      auto & src_rng = rngs[ipipe].transitions;
      for (auto & i : irngs_prev) {
        if (&rngs[i].transitions != &src_rng) {
          rngs[i].transitions.insert(rngs[i].transitions.end(), src_rng.begin(), src_rng.end());
        }
      }
    };
    
    auto merge_group = [this]{
      auto & irngs_prev = stack.back().irngs;
      auto const superimposed_idx = (irngs_prev.back() == irngs.front()) ? 1 : 0; 
      irngs_prev.insert(irngs_prev.end(), irngs.begin() + superimposed_idx, irngs.end());
      irngs = std::move(irngs_prev);
    };
    
    c = consumer.bumpc();
    
    switch (c) {
      case '+': scan_multi_one_or_more(); copy_first(); if (has_ipipe) merge_group(); break;
      case '*': scan_multi_zero_or_more(); copy_first(); merge_group(); break;
      case '?': scan_multi_optional(); copy_first(); merge_group(); break;
      case '{': {
        bool m = scan_multi_interval(); 
        copy_first(); 
        if (m) {
          merge_group(); 
        }
        else if (has_ipipe) {
          merge_group(); 
        }
      }
      break;
      default: copy_first(); if (has_ipipe) merge_group();
    }

    ipipe = stack[stack.size()-2].ipipe;
    iends = stack.back().iends;
    stack.pop_back();

    if (cap_stack.is_marked(cap_level())) {
      auto n = cap_stack.unmark(cap_level());
      for (auto i : irngs) {
        auto & c = rngs[i].capstates;
        if (!c.empty() && c.back().n == n) {
          c.back().e |= Capture::Close;
        }
        else {
          c.push_back({n, Capture::Close});
        }
      }
    }
  }

  void scan_begin() {
    if (irngs.front() != 0) {
      ts.clear();
      for (auto & i : irngs) {
        rngs[i].transitions.push_back({{char_int{}, ~char_int{}}, count_rngs()});
        rngs[i].states |= Range::Invalid;
      }
      irngs.clear();
      new_range(std::false_type{});
    }
    states |= Range::Begin;
    c = consumer.bumpc();
  }

  void scan_end() {
    for (auto i : irngs) {
      rngs[i].states |= Range::End;
      rngs[i].states &= ~Range::Normal;
    }
    c = consumer.bumpc();
  }

  void scan_normal() {
    Event const e = [&]() {
      if (c == '.') {
        return Event{char_int{}, ~char_int{}};
      }
      if (c == '\\') {
        if (!(c = consumer.bumpc())) {
          throw std::runtime_error("error end of string");
        }
        // TODO character class
      }
      return Event{c, c};
    }();

    ts.clear();
    auto const n = count_rngs();
    ts.push_back({e, n});
    c = consumer.bumpc();
  }

  void scan_single_one_or_more() {
    set_transitions();
    irngs.clear();
    irngs.push_back(count_rngs());
    new_range(std::true_type{});
    rngs.back().transitions = ts;
    c = consumer.bumpc();
  }

  void scan_single_zero_or_more() {
    set_transitions();
    irngs.push_back(count_rngs());
    {
      auto has_begin = Range::Begin & states;
      new_range(std::false_type{});
      states |= has_begin;
    }
    rngs.back().transitions = ts;
    c = consumer.bumpc();
  }

  void scan_single_optional() {
    set_transitions();
    irngs.push_back(count_rngs());
    {
      auto has_begin = Range::Begin & states;
      new_range(std::false_type{});
      states |= has_begin;
    }
    c = consumer.bumpc();
  }

  void scan_single_interval() {
    auto start = consumer.str();
    char * end = 0;

    auto repeat_element = [&](unsigned long n, auto pre_push) {
      set_transitions();
      pre_push();
      irngs.push_back(count_rngs());
      new_range(std::true_type{});
      --n;
      while (n--) {
        renext_transitions();
        set_transitions();
        pre_push();
        irngs.push_back(count_rngs());
        new_range(std::false_type{});
      }
    };
     
    auto repeat_conditional = [&](auto check) {
      unsigned long n = strtoul(start, &end, 10);

      if (end == start) {
        throw std::runtime_error("invalid syntaxe {,}");
      }
      if (!n) {
        throw std::runtime_error("numbers is 0 in {}");
      }
      if (!(n = check(n))) {
        return ;
      }

      repeat_element(n, []{});
    };

    // {,n}
    if (*start == ',') {
      ++start;
      repeat_conditional([](auto n) {return n;});
    }
    // {m...}
    else {
      unsigned long m = strtoul(start, &end, 10);

      if (end == start) {
        throw std::runtime_error("invalid syntaxe in {}");
      }
      if (0u == m) {
        throw std::runtime_error("numbers is 0 in {}");
      }

      repeat_element(m, [&]{irngs.clear();});

      if (*end == ',') {
        // {m,}
        if (*++end == '}') {
          rngs.back().transitions = ts;
        }
        // {m,n}
        else {
          start = end;
          renext_transitions();
          repeat_conditional([&](auto n) {
            if (m > n) {
              throw std::runtime_error("numbers out of order in {} quantifier");
            }
            return n-m;
          });
        }
      }
    }
    if (*end != '}') {
      throw std::runtime_error("unmatched }");
    }
    consumer.str(end + 1);
    c = consumer.bumpc();
  }

  void add_single_range() {
    set_transitions();
    irngs.clear();
    irngs.push_back(count_rngs());
    new_range(std::true_type{});
  }
};

}

Ranges scan(const char * s)
{
  basic_scanner scanner;
  scanner.prepare();
  scanner.scan(s);
  return scanner.final();
}

}
}