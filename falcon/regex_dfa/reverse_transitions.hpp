#ifndef FALCON_REGEX_DFA_REVERSE_TRANSITIONS_HPP
#define FALCON_REGEX_DFA_REVERSE_TRANSITIONS_HPP

namespace falcon { namespace regex_dfa {

class Transitions;
void reverse_transitions(Transitions &, unsigned n);

} }

#endif
