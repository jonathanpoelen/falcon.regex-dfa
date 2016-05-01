#ifndef FALCON_REGEX_DFA_REVERSE_TRANSITIONS_HPP
#define FALCON_REGEX_DFA_REVERSE_TRANSITIONS_HPP

#include "redfa.hpp"

namespace falcon { namespace regex_dfa {

void reverse_transitions(Transitions &, unsigned n, Transition::State state);

} }

#endif
