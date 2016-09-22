/* The MIT License (MIT)

Copyright (c) 2016 jonathan poelen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
* \author    Jonathan Poelen <jonathan.poelen+falcon@gmail.com>
* \version   0.1
*/

#pragma once

#include <vector>
#include <iosfwd>
#include <cstdint>


namespace falcon { namespace regex {

using char_int = uint32_t;
using size_type = std::size_t;

namespace detail { namespace {
  enum _regex_state_quanti {
    none,
    closure0,
    closure1,
    option
  };
} }

enum class regex_state : char
{
  start,
  start_alternation,
  bol,
  eol,
  open,
  open_alternation,
  open_nocap,
  open_nocap_alternation,
  alternation,
  terminate,

  brace0, // {,m}
  repetition, // {n}
  brace1, // {n,}
  interval, // {n,m}

  single1,
  single1_closure0,
  single1_closure1,
  single1_option,

  escaped,
  escaped_closure0,
  escaped_closure1,
  escaped_option,

  any,
  any_closure0,
  any_closure1,
  any_option,

  bracket,
  bracket_closure0,
  bracket_closure1,
  bracket_option,

  bracket_reverse,
  bracket_reverse_closure0,
  bracket_reverse_closure1,
  bracket_reverse_option,

  close,
  close_closure0,
  close_closure1,
  close_option,

  NB
};

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> &
operator <<(std::basic_ostream<Ch, Tr> & os, regex_state const & st) {
  constexpr char const * names[]{
    "start",
    "start_alternation",
    "bol",
    "eol",
    "open",
    "open_alternation",
    "open_nocap",
    "open_nocap_alternation",
    "alternation",
    "terminate",

    "brace0",
    "repetition",
    "brace1",
    "interval",

    "single1",
    "single1_closure0",
    "single1_closure1",
    "single1_option",

    "escaped",
    "escaped_closure0",
    "escaped_closure1",
    "escaped_option",

    "any",
    "any_closure0",
    "any_closure1",
    "any_option",

    "bracket",
    "bracket_closure0",
    "bracket_closure1",
    "bracket_option",

    "bracket_reverse",
    "bracket_reverse_closure0",
    "bracket_reverse_closure1",
    "bracket_reverse_option",

    "close",
    "close_closure0",
    "close_closure1",
    "close_option",
  };
  static_assert(sizeof(names)/sizeof(names[0]) == static_cast<std::size_t>(regex_state::NB), "");
  return os << names[static_cast<unsigned>(st)];
}

struct scanner_ctx
{
  struct start_t
  {
    unsigned ibeg;
    unsigned iend;
  };

  struct open_t
  {
    unsigned ibeg;
    unsigned iend;
    unsigned idx_close;
  };

  struct close_t
  {
    unsigned idx_open;
  };

  struct single_t
  {
    char_int c;
  };

  struct bracket_t
  {
    unsigned ibeg;
    unsigned iend;
  };

  struct interval_t
  {
    unsigned n;
    unsigned m;
  };

  struct elem_t
  {
    regex_state state;
    union U
    {
      start_t start;
      open_t open;
      close_t close;
      single_t single;
      bracket_t bracket;
      interval_t interval;
      struct {} dummy;
    } data;

    elem_t(regex_state st)
    : state(st), data()
    {}
  };

  // TODO char_int or size_type
  using param_type = uint32_t;

  // idx_alternation... [ idx_first_altern, elems.size, idx_alternation... ] ...
  std::vector<char_int> bracket_list;
  std::vector<unsigned> alter_list;
  std::vector<unsigned> stack_alter_list;
  std::vector<param_type> stack_params;
  std::vector<elem_t> elems;
};

scanner_ctx scan(char const * s);

} }
