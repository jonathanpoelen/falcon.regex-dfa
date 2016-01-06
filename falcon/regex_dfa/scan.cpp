#include "scan.hpp"
#include "scan_intervals.hpp"

#include <stdexcept>
#include <algorithm>

// #include <iostream>
// #include "print_automaton.hpp"

namespace {

struct Stack {
  std::vector<unsigned> iends;
  std::vector<unsigned> irngs;
  falcon::regex_dfa::Range::State states;
  unsigned icap;
  unsigned itrs;
};

class CapStack
{
  using cap_flags_t = std::size_t;
  static constexpr std::size_t max_cap_level = sizeof(cap_flags_t) * 8;
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
}

falcon::regex_dfa::Ranges falcon::regex_dfa::scan(const char * s)
{
  std::vector<Stack> stack;
  stack.push_back({{}, {}, Range::Normal, 0, 0});

  CapStack cap_stack;

  auto cap_level = [&]() { return stack.size() - 1; };

  Ranges rngs;
  std::vector<unsigned> irngs;
  std::vector<unsigned> iends;
  unsigned ipipe = 0;
  irngs.push_back(0);

  Range::State states = Range::Normal;
  auto new_range = [&](auto remove_open_cap) -> Range & {
    rngs.push_back({states, cap_stack.captures(), {}});
    if (remove_open_cap) {
      cap_stack.remove_open();
    }
    states = Range::Normal;
    return rngs.back();
  };
  new_range(std::false_type{});

  Transitions ts;

  auto set_transitions = [&]{
    for (auto & i : irngs) {
      rngs[i].transitions.insert(rngs[i].transitions.end(), ts.begin(), ts.end());
    }
  };

  auto count_rngs = [&]() { return static_cast<unsigned>(rngs.size()); };

  auto renext_transitions = [&]{
    for (auto && t : ts) {
      t.next = count_rngs();
    }
  };

  enum class MultiDup { Repeat, Conditional, RepeatAndConditional, RepeatAndMore };

#ifdef IN_IDE_PARSER
  Captures tmp_capstates;
  std::vector<Range> new_rng;
  auto repeat_multi_dup = [&]
#else
  auto repeat_multi_dup = [&, tmp_capstates = Captures(), new_rng = std::vector<Range>()]
#endif
  (unsigned long m, unsigned long n, auto estate) mutable
  {
    auto const sz = rngs.size();
    new_rng.assign(&rngs[ipipe/*s.back()*/], &rngs[sz]);
    auto count_rng_added = unsigned(new_rng.size() - 1u);
    {
      auto & ts = rngs[ipipe/*s.back()*/].transitions;
      ts.erase(ts.begin(), ts.begin() + stack.back().itrs);
    }

    auto const ncp_irng = static_cast<unsigned>(std::partition(
      irngs.begin()
    , irngs.end()
    , [&](auto i) {
      return i <= ipipe;
    }) - irngs.begin());

    decltype(irngs) cp_irngs;
    decltype(cp_irngs) tmp_irng;
    if (estate == MultiDup::Conditional) {
      cp_irngs.assign(irngs.begin() + ncp_irng, irngs.end());
    }

    auto set_union = [](auto & c, auto const & c2, auto & tmp, auto cmp) {
      tmp.resize(c.size() + c2.size());
      tmp.erase(
        std::set_union(
          c.begin(), c.end(),
          c2.begin(), c2.end(),
          tmp.begin(), cmp
        ),
        tmp.end()
      );
      using std::swap;
      swap(c, tmp);
    };

    auto update_transition_indexes = [&] {
      std::for_each(new_rng.begin(), new_rng.end(), [&](auto & r) {
        for (auto && t : r.transitions) {
          t.next += count_rng_added;
        }
      });
    };
    
    std::vector<unsigned> extended_irng;
    std::for_each(irngs.begin(), irngs.begin() + ncp_irng, [&](unsigned i) {
      Transitions const & ts = rngs[i].transitions;
      for (auto & t : ts) {
        if (std::find(irngs.begin(), irngs.end(), t.next) != irngs.end()) {
          extended_irng.push_back(static_cast<unsigned>(t.next));
        }
      }
    });
    
    auto insert_transitions = [&](Transitions & transitions_added){
      for (auto & i : irngs) {
        Range & r = rngs[i];
        r.states |= states;
        set_union(r.capstates, cap_stack.captures(), tmp_capstates, std::less<>{});
        r.transitions.insert(r.transitions.end(), transitions_added.begin(), transitions_added.end());
       }
    };

    auto update_rngs = [&] {
      update_transition_indexes();
      rngs.insert(rngs.end(), new_rng.begin() + 1, new_rng.end());
      insert_transitions(new_rng[0].transitions);
      for (auto p = irngs.begin() + ncp_irng, e = irngs.end(); p != e; ++p) {
        *p += count_rng_added;
      }
      irngs.insert(irngs.end(), extended_irng.begin(), extended_irng.end());
    };

    while (--m) {
      if (estate == MultiDup::Conditional) {
        irngs.push_back(ipipe);
        set_union(irngs, cp_irngs, tmp_irng, std::greater<>{});
      }
      update_rngs();
    }

    if (estate == MultiDup::RepeatAndMore) {
      ts = new_rng[0].transitions;
    }

    if (estate == MultiDup::Conditional) {
      irngs.push_back(ipipe);
    }

    if (estate != MultiDup::RepeatAndConditional) {
      return ;
    }

    if (!n) {
      return ;
    }

    cp_irngs.assign(irngs.begin() + ncp_irng, irngs.end());

    while (n--) {
      // TODO irngs => irngs[ncp_irng..end]
      set_union(cp_irngs, irngs, tmp_irng, std::greater<>{});
      update_rngs();
    }
    set_union(irngs, cp_irngs, tmp_irng, std::greater<>{});
  };


  utf8_consumer consumer(s);
  char_int c = consumer.bumpc();

  auto scan_bracket = [&]{
    ts.clear();
    scan_intervals(consumer, ts, count_rngs());
    c = consumer.bumpc();
  };

  auto scan_or = [&]{
    iends.insert(iends.end(), irngs.begin(), irngs.end());
    irngs.clear();
    irngs.push_back(ipipe);
    c = consumer.bumpc();
    states = stack.back().states;
    cap_stack.alternation();
  };

  auto scan_multi_one_or_more = [&]{
    ts = rngs[stack.back().icap].transitions;
    set_transitions();
    c = consumer.bumpc();
  };

  auto scan_multi_optional = [&]{
    c = consumer.bumpc();
  };

  auto scan_multi_zero_or_more = [&]{
    ts = rngs[stack.back().icap].transitions;
    set_transitions();
    c = consumer.bumpc();
  };

  auto scan_multi_interval = [&]{
    auto start = consumer.str();
    char * end = 0;

    auto exec_state = [&](unsigned long m, auto estate, auto check) {
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

      repeat_multi_dup(m, n, estate);
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
  };

  auto scan_open = [&]{
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
      {iends},
      std::move(irngs),
      states,
      ipipe,
      unsigned(rngs[ipipe].transitions.size())
    });
    iends.clear();
    irngs.clear();
    irngs.push_back(ipipe);
  };

  auto scan_close = [&]{
    if (!cap_level()) {
      throw std::runtime_error("unmatched )");
    }
    
    auto copy_first = [&rngs, &stack]{
      auto & irngs_prev = stack.back().irngs;
      auto & src_rng = rngs[stack.back().icap].transitions;
      for (auto & i : irngs_prev) {
        if (&rngs[i].transitions != &src_rng) {
          rngs[i].transitions.insert(rngs[i].transitions.end(), src_rng.begin(), src_rng.end());
        }
      }
    };
    
    auto merge_group = [&irngs, &stack]{
      auto & irngs_prev = stack.back().irngs;
      irngs_prev.insert(irngs_prev.end(), irngs.begin(), irngs.end());
      irngs = std::move(irngs_prev);
    };
    
    auto merge_group_if = [&irngs, &stack, &merge_group]{
      // ngs[stack.back().icap] and iend
      if (irngs.end() != std::find(irngs.begin(), irngs.end(), stack.back().icap)) {
        merge_group();
      }
    };

    c = consumer.bumpc();
    irngs.insert(irngs.end(), iends.begin(), iends.end());
    switch (c) {
      case '+': scan_multi_one_or_more(); copy_first(); merge_group_if(); break;
      case '*': scan_multi_zero_or_more(); copy_first(); merge_group(); break;
      case '?': scan_multi_optional(); copy_first(); merge_group(); break;
      case '{': {
        bool m = scan_multi_interval(); 
        copy_first(); 
        if (m) {
          merge_group(); 
        }
        else {
          merge_group_if(); 
        }
      }
      break;
      default: copy_first(); merge_group_if();
    }

    ipipe = stack[stack.size()-2].icap;
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
  };

  auto scan_begin = [&]{
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
  };

  auto scan_end = [&]{
    for (auto i : irngs) {
      rngs[i].states |= Range::End;
      rngs[i].states &= ~Range::Normal;
    }
    c = consumer.bumpc();
  };

  auto scan_normal = [&]{
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
  };

  auto scan_single_one_or_more = [&]{
    set_transitions();
    irngs.clear();
    irngs.push_back(count_rngs());
    new_range(std::true_type{});
    rngs.back().transitions = ts;
    c = consumer.bumpc();
  };

  auto scan_single_zero_or_more = [&]{
    set_transitions();
    irngs.push_back(count_rngs());
    {
      auto has_begin = Range::Begin & states;
      new_range(std::false_type{});
      states |= has_begin;
    }
    rngs.back().transitions = ts;
    c = consumer.bumpc();
  };

  auto scan_single_optional = [&]{
    set_transitions();
    irngs.push_back(count_rngs());
    {
      auto has_begin = Range::Begin & states;
      new_range(std::false_type{});
      states |= has_begin;
    }
    c = consumer.bumpc();
  };

  auto scan_single_interval = [&]{
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
  };

  auto add_single_range = [&]{
    set_transitions();
    irngs.clear();
    irngs.push_back(count_rngs());
    new_range(std::true_type{});
  };

  auto check_no_repetition = [&]{
    switch (c) {
      case '?': case '+': case '*': case '{':
        throw std::runtime_error("syntaxe error ? + * {");
    }
  };

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


  for (auto ap : {&iends, &irngs}) {
    for (auto i : *ap) {
      if (!(rngs[i].states & Range::End)) {
        rngs[i].states |= Range::Final | states;
        rngs[i].states &= ~Range::Normal;
      }
    }
  }

  rngs.capture_table = std::move(cap_stack.capture_table);

  return rngs;
}
