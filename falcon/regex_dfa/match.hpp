#ifndef FALCON_REGEX_DFA_MATCH_HPP
#define FALCON_REGEX_DFA_MATCH_HPP

namespace falcon { namespace regex_dfa {

class Ranges;

bool match(Ranges const & rngs, char const * s);
bool nfa_match(Ranges const & rngs, char const * s);

} }

#endif