#ifndef FALCON_REGEX_DFA_RANGE_ITERATOR_HPP
#define FALCON_REGEX_DFA_RANGE_ITERATOR_HPP

#include <iterator>

namespace falcon {

template<class It>
struct range_iterator
{
  It first, last;
  It begin() const { return first; }
  It end() const { return last; }
  std::size_t size() const { return last - first; }
};

template<class It>
range_iterator<It> make_range(It first, It last) {
  return {first, last};
}

using std::rbegin;
using std::rend;
  
template<class Cont>
auto make_reverse_range(Cont & cont) 
-> range_iterator<decltype(rbegin(cont))> {
  return {rbegin(cont), rend(cont)};
}

}

#endif
