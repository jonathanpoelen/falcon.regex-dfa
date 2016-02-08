#include "falcon/regex_dfa/scan.hpp"
#include "falcon/regex_dfa/match.hpp"
#include "falcon/regex_dfa/print_automaton.hpp"

#include <iostream>

unsigned count_test_failure = 0;

namespace re = falcon::regex_dfa;

void test(
  char const * pattern
, char const * s
, bool is_ok
, unsigned line
) {
  re::Ranges const & rngs = re::scan(pattern);
  if (re::nfa_match(rngs, s) != is_ok) {
    std::cerr
      << ++count_test_failure << "  line: " << line
      << "\n\n pattern: \033[37;02m" << pattern
      << "\n\033[0m str: \033[37;02m" << s
      << "\n\033[0m expected match: " << is_ok
      << "\n\n"
    ;
    re::print_automaton(rngs);
    std::cerr << "----------\n";
  }
}

#define YES(pattern, s) test(pattern, s, true, __LINE__)
#define NO(pattern, s) test(pattern, s, false, __LINE__)

int main() {

  YES("a", "aaaa");
  YES("a", "a");
  YES("a", "ab");
  YES("a", "abc");
  NO("a", "");
  NO("a", "b");
  NO("a", "ba");

  YES("^a", "aaaa");
  YES("^a", "a");
  YES("^a", "ab");
  YES("^a", "abc");
  NO("^a", "");
  NO("^a", "b");
  NO("^a", "ba");

  NO("a^b", "a");
  NO("a^b", "");
  NO("a^b", "ba");
  NO("a^b", "b");
  NO("a^b", "ab");

  YES("a$", "a");
  NO("a$", "");
  NO("a$", "ba");
  NO("a$", "aaaa");
  NO("a$", "aaaab");

  YES(".*a$", "a");
  YES(".*a$", "ba");
  YES(".*a$", "bba");
  YES(".*a$", "baa");
  YES(".*a$", "aaaa");
  NO(".*a$", "aaaab");
  NO(".*a$", "");

  YES("^a$", "a");
  NO("^a$", "aaaa");
  NO("^a$", "ba");
  NO("^a$", "");
  NO("^a$", "aaaab");
  NO("^a$", "ab");
  NO("^a$", "abc");
  NO("^a$", "b");

  YES("^a|^.?", "a");
  YES("^a|^.?", "");
  NO("^a|^.?", "aaaa");
  NO("^a|^.?", "ba");

  YES("a+", "aaaa");
  YES("a+", "a");
  NO("a+", "ab");
  NO("a+", "abc");
  NO("a+", "baaa");
  NO("a+", "ba");
  NO("a+", "");
  NO("a+", "b");

  YES(".*a.*$", "aaaa");
  YES(".*a.*$", "a");
  YES(".*a.*$", "baaa");
  YES(".*a.*$", "ba");
  YES(".*a.*$", "ab");
  YES(".*a.*$", "abc");
  YES(".*a.*$", "dabc");
  NO(".*a.*$", "");
  NO(".*a.*$", "b");

  YES("aa+$", "aaaa");
  NO("aa+$", "baaa");
  NO("aa+$", "baa");
  NO("aa+$", "abc");
  NO("aa+$", "a");
  NO("aa+$", "b");
  NO("aa+$", "ba");
  NO("aa+$", "ab");
  NO("aa+$", "");

  YES(".*aa+$", "aaaa");
  YES(".*aa+$", "baaa");
  YES(".*aa+$", "baa");
  NO(".*aa+$", "abc");
  NO(".*aa+$", "a");
  NO(".*aa+$", "ba");
  NO(".*aa+$", "ab");
  NO(".*aa+$", "dabc");
  NO(".*aa+$", "");
  NO(".*aa+$", "b");

  YES("aa*b?a", "aa");
  YES("aa*b?a", "aaaa");
  YES("aa*b?a", "aba");
  YES("aa*b?a", "aaba");
  YES("aa*b?a", "aba");
  NO("aa*b?a", "baaab");
  NO("aa*b?a", "baabaaa");
  NO("aa*b?a", "");
  NO("aa*b?a", "ab");
  NO("aa*b?a", "abc");
  NO("aa*b?a", "bba");
  NO("aa*b?a", "b");
  NO("aa*b?a", "dabc");

  YES("[a-cd]", "a");
  YES("[a-cd]", "b");
  YES("[a-cd]", "c");
  YES("[a-cd]", "d");
  NO("[a-cd]", "l");
  NO("[a-cd]", "");

  YES("[^a-cd]", "l");
  NO("[^a-cd]", "a");
  NO("[^a-cd]", "b");
  NO("[^a-cd]", "c");
  NO("[^a-cd]", "d");
  NO("[a-cd]", "");

  YES("a|b", "a");
  YES("a|b", "b");
  NO("a|b", "c");

  YES("^()$", "");
  NO("^()$", "a");

  YES("^(a)$", "a");
  NO("^(a)$", "aa");

  YES("(a|b)|c", "a");
  YES("(a|b)|c", "b");
  YES("(a|b)|c", "c");
  NO("(a|b)|c", "d");

  YES("^(a|b)|c$", "a");
  YES("^(a|b)|c$", "b");
  YES("^(a|b)|c$", "c");
  NO("^(a|b)|c$", "au");
  NO("^(a|b)|c$", "ua");
  NO("^(a|b)|c$", "d");

  YES("(a)+", "a");
  YES("(a)+", "aa");
  YES("(a)+", "aaa");
  YES("(a)+", "ab");
  NO("(a)+", "");
  NO("(a)+", "b");
  NO("(a)+", "ba");

  YES("(a)*", "");
  YES("(a)*", "a");
  YES("(a)*", "aa");
  NO("(a)*", "b");
  NO("(a)*", "ba");
  NO("(a)*", "ab");

  YES("(a)?", "");
  YES("(a)?", "a");
  YES("(a)?", "aa");
  YES("(a)?", "aaa");
  NO("(a)?", "ba");
  NO("(a)?", "ab");
  NO("(a)?", "b");

  YES("b(a)*", "b");
  YES("b(a)*", "ba");
  YES("b(a)*", "baa");
  NO("b(a)*", "");
  NO("b(a)*", "a");
  NO("b(a)*", "aa");
  NO("b(a)*", "ab");

  YES("b(a)?", "ba");
  YES("b(a)?", "b");
  NO("b(a)?", "baa");
  NO("b(a)?", "");
  NO("b(a)?", "a");
  NO("b(a)?", "aa");
  NO("b(a)?", "ab");

  YES("b(a)*c", "bc");
  YES("b(a)*c", "bac");
  YES("b(a)*c", "baaac");
  NO("b(a)*c", "abc");
  NO("b(a)*c", "b");
  NO("b(a)*c", "ba");
  NO("b(a)*c", "ab");
  NO("b(a)*c", "b");
  NO("b(a)*c", "");
  NO("b(a)*c", "a");
  NO("b(a)*c", "aa");

  YES("b(a)?c", "bac");
  YES("b(a)?c", "bc");
  NO("b(a)?c", "baac");
  NO("b(a)?c", "abc");
  NO("b(a)?c", "ba");
  NO("b(a)?c", "ab");
  NO("b(a)?c", "b");
  NO("b(a)?c", "");
  NO("b(a)?c", "a");
  NO("b(a)?c", "aa");

  YES("b(a?)?c", "bac");
  YES("b(a?)?c", "bc");
  NO("b(a?)?c", "baac");
  NO("b(a?)?c", "abc");
  NO("b(a?)?c", "ba");
  NO("b(a?)?c", "ab");
  NO("b(a?)?c", "b");
  NO("b(a?)?c", "");
  NO("b(a?)?c", "a");
  NO("b(a?)?c", "aa");

  YES("(ab)+", "ab");
  YES("(ab)+", "abab");
  YES("(ab)+", "ababab");
  NO("(ab)+", "aba");
  NO("(ab)+", "bab");
  NO("(ab)+", "");
  NO("(ab)+", "b");
  NO("(ab)+", "a");

  YES("(ab)*", "");
  YES("(ab)*", "ab");
  YES("(ab)*", "abab");
  YES("(ab)*", "ababab");
  NO("(ab)*", "aba");
  NO("(ab)*", "a");
  NO("(ab)*", "b");
  NO("(ab)*", "c");

  YES("(ab)?", "");
  YES("(ab)?", "ab");
  NO("(ab)?", "aba");
  NO("(ab)?", "abab");
  NO("(ab)?", "a");
  NO("(ab)?", "b");
  NO("(ab)?", "c");

  YES("c(ab)+", "cab");
  YES("c(ab)+", "cabab");
  YES("c(ab)+", "cababab");
  NO("c(ab)+", "caba");
  NO("c(ab)+", "");
  NO("c(ab)+", "c");
  NO("c(ab)+", "b");
  NO("c(ab)+", "cb");
  NO("c(ab)+", "a");
  NO("c(ab)+", "ca");

  YES("c(ab)*", "c");
  YES("c(ab)*", "cab");
  YES("c(ab)*", "cabab");
  NO("c(ab)*", "caba");
  NO("c(ab)*", "ca");
  NO("c(ab)*", "cb");
  NO("c(ab)*", "cc");
  NO("c(ab)*", "");

  YES("c(ab)?", "c");
  YES("c(ab)?", "cab");
  YES("c(ab)?", "c");
  NO("c(ab)?", "ca");
  NO("c(ab)?", "cabab");
  NO("c(ab)?", "");
  NO("c(ab)?", "a");
  NO("c(ab)?", "b");

  YES("c(ab)+d", "cabd");
  YES("c(ab)+d", "cababd");
  YES("c(ab)+d", "cabababd");
  NO("c(ab)+d", "cabad");
  NO("c(ab)+d", "bcabd");
  NO("c(ab)+d", "cabababb");
  NO("c(ab)+d", "cad");
  NO("c(ab)+d", "cbd");
  NO("c(ab)+d", "cabbd");
  NO("c(ab)+d", "cababbb");

  YES("c(ab)*d", "cd");
  YES("c(ab)*d", "cabd");
  YES("c(ab)*d", "cababd");
  NO("c(ab)*d", "cabad");
  NO("c(ab)*d", "");
  NO("c(ab)*d", "cad");
  NO("c(ab)*d", "cbd");
  NO("c(ab)*d", "cabad");

  YES("c(ab)?d", "cd");
  YES("c(ab)?d", "cabd");
  NO("c(ab)?d", "cabad");
  NO("c(ab)?d", "");
  NO("c(ab)?d", "cad");
  NO("c(ab)?d", "cbd");
  NO("c(ab)?d", "cabad");
  NO("c(ab)?d", "cababd");

  YES("(ab){2}", "abab");
  NO("(ab){2}", "aba");
  NO("(ab){2}", "aababa");
  NO("(ab){2}", "aba");
  NO("(ab){2}", "ab");

  YES("(ab){3}", "ababab");
  NO("(ab){3}", "abababa");
  NO("(ab){3}", "aabababa");
  NO("(ab){3}", "aba");
  NO("(ab){3}", "ababa");
  NO("(ab){3}", "ab");

  YES("^b(a?){3}$", "b");
  YES("^b(a?){3}$", "ba");
  YES("^b(a?){3}$", "baa");
  YES("^b(a?){3}$", "baaa");
  NO("^b(a?){3}$", "baaaa");
  NO("^b(a?){3}$", "bb");
  NO("^b(a?){3}$", "bab");
  NO("^b(a?){3}$", "baab");

  YES("^(ab){1,}$", "ab");
  YES("^(ab){1,}$", "abab");
  YES("^(ab){1,}$", "ababab");
  NO("^(ab){1,}$", "ababa");
  NO("^(ab){1,}$", "baba");

  YES("^(ab){,2}$", "ab");
  YES("^(ab){,2}$", "abab");
  NO("^(ab){,2}$", "ababa");
  NO("^(ab){,2}$", "ababab");
  NO("^(ab){,2}$", "ababa");
  NO("^(ab){,2}$", "baba");

  YES("^(ab){2,4}$", "abab");
  YES("^(ab){2,4}$", "ababab");
  YES("^(ab){2,4}$", "abababab");
  NO("^(ab){2,4}$", "ababababa");
  NO("^(ab){2,4}$", "ababababab");
  NO("^(ab){2,4}$", "ab");

  YES("^(a+|b+|cd*)$", "aaa");
  YES("^(a+|b+|cd*)$", "bbb");
  YES("^(a+|b+|cd*)$", "c");
  YES("^(a+|b+|cd*)$", "cdd");
  NO("^(a+|b+|cd*)$", "aaab");
  NO("^(a+|b+|cd*)$", "acdd");

  YES("^(?!a+|b+|cd*){3}$", "aaa");
  YES("^(?!a+|b+|cd*){3}$", "bbb");
  YES("^(?!a+|b+|cd*){3}$", "abc");
  YES("^(?!a+|b+|cd*){3}$", "cddaa");
  YES("^(?!a+|b+|cd*){3}$", "aacddb");
  YES("^(?!a+|b+|cd*){3}$", "ccc");
  NO("^(?!a+|b+|cd*){3}$", "c");
  NO("^(?!a+|b+|cd*){3}$", "cc");
  NO("^(?!a+|b+|cd*){3}$", "ab");

  if (count_test_failure) {
    std::cerr << "error(s): " << count_test_failure << "\n";
  }
  return count_test_failure ? 1 : 0;
}
