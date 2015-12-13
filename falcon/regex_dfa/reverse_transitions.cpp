#include "reverse_transitions.hpp"
#include "redfa.hpp"

#include <algorithm>

void falcon::regex_dfa::reverse_transitions(Transitions& ts, unsigned n)
{
  if (ts.empty()) {
    ts.push_back({{char_int{}, ~char_int{}}, n});
    return;
  }

  std::sort(ts.begin(), ts.end(), [](Transition & t1, Transition & t2) {
    return t1.e.l < t2.e.l;
  });
  auto pred = [](Transition & t1, Transition & t2) {
    return t2.e.l <= t1.e.r || t1.e.r + 1 == t2.e.l;
  };
  auto first = std::adjacent_find(ts.begin(), ts.end(), pred);
  auto last = ts.end();
  if (first != last) {
    auto result = first;
    result->e.r = std::max(result->e.r, first->e.r);
    while (++first != last) {
      if (pred(*result, *first)) {
        result->e.r = std::max(result->e.r, first->e.r);
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

  if (ts.front().e.l) {
    auto c1 = first->e.r;
    first->e.r = first->e.l-1;
    first->e.l = 0;
    for (; ++first !=last; ) {
      auto c2 = first->e.l;
      first->e.l = c1+1;
      c1 = first->e.r;
      first->e.r = c2-1;
    }
    if (c1 != ~char_int{}) {
      ts.push_back({{++c1, ~char_int{}}, n});
    }
  }
  else {
    --last;
    for (; first != last; ++first) {
      first->e.l = first->e.r + 1;
      first->e.r = (first+1)->e.l - 1;
    }
    if (first->e.r != ~char_int{}) {
      first->e.r = ~char_int{};
    }
    else {
      ts.pop_back();
    }
  }
}
