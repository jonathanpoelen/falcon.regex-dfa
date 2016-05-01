#if !defined(FALCON_REGEX_DFA_TRACE) \
  && defined(FALCON_REGEX_DFA_ENABLE_TRACE) && FALCON_REGEX_DFA_ENABLE_TRACE != 0 \
  || defined(IN_IDE_PARSER)
# define FALCON_REGEX_DFA_TRACE void
# include "print_automaton.hpp"
# include <iostream>
#else
# define FALCON_REGEX_DFA_TRACE(...) void()
#endif

#define FALCON_REGEX_DFA_TRACE_VAR(name) \
  FALCON_REGEX_DFA_TRACE(std::cerr << #name ": " << name << "\n")

#define FALCON_REGEX_DFA_TRACE_VAR2(name, value) \
  FALCON_REGEX_DFA_TRACE(std::cerr << #name ": " << value << "\n")

// TODO falcon/iostreams/join.hpp
namespace {
  template<class Cont> struct Join_ { Cont const & cont; };
  template<class Cont> Join_<Cont> join_(Cont const & cont) { return {cont}; }
  template<class Cont> std::ostream & operator << (std::ostream & os, Join_<Cont> const & j) {
    for (auto const & x : j.cont) {
      os << x << " ";
    }
    return os;
  }
}

#define FALCON_REGEX_DFA_TRACE_RNG(name) \
  FALCON_REGEX_DFA_TRACE(std::cerr << #name ": " << join_(name) << "\n")

#define FALCON_REGEX_DFA_TRACE_FUNC() \
  FALCON_REGEX_DFA_TRACE(std::cerr << "# " << __func__ << "\n")

#define FALCON_REGEX_DFA_TRACE_FUNC_VAR(name)                      \
  FALCON_REGEX_DFA_TRACE(std::cerr << "# " << __func__ << " -- "); \
  FALCON_REGEX_DFA_TRACE_VAR(name)

#define FALCON_REGEX_DFA_TRACE_FUNC_VAR2(name, value)              \
  FALCON_REGEX_DFA_TRACE(std::cerr << "# " << __func__ << " -- "); \
  FALCON_REGEX_DFA_TRACE_VAR2(name, value)

#define FALCON_REGEX_DFA_TRACE_FUNC_RNG(name)                      \
  FALCON_REGEX_DFA_TRACE(std::cerr << "# " << __func__ << " -- "); \
  FALCON_REGEX_DFA_TRACE_RNG(name)
