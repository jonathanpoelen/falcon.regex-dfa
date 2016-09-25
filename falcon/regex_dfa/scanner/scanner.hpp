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
using size_type = unsigned;

enum class regex_state : char
{
  start,
  alternation,
  bol,
  eol,
  open,
  open_nocap,
  terminate,

  brace0, // {,m} -> {0,m}
  repetition, // {n}
  brace1, // {n,}
  interval, // {n,m}
  closure0, // *
  closure1, // +
  option, // ?

  single1,
  escaped,
  any,
  bracket,
  bracket_reverse,
  close,

  NB
};

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> &
operator <<(std::basic_ostream<Ch, Tr> & os, regex_state const & st) {
  constexpr char const * names[]{
    "start",
    "alternation",
    "bol",
    "eol",
    "open",
    "open_nocap",
    "terminate",

    "brace0",
    "repetition",
    "brace1",
    "interval",
    "closure0",
    "closure1",
    "option",

    "single1",
    "escaped",
    "any",
    "bracket",
    "bracket_reverse",
    "close",
  };
  static_assert(sizeof(names)/sizeof(names[0]) == static_cast<std::size_t>(regex_state::NB), "");
  return os << names[static_cast<unsigned>(st)];
}

struct scanner_ctx
{
  struct alternation_t
  {
    size_type inc_next;
  };

  struct open_t
  {
    size_type inc_close;
    size_type inc_last_alternation;
  };

  struct close_t
  {
    size_type dec_open;
  };

  struct single_t
  {
    char_int c;
  };

  struct bracket_t
  {
    size_type ibeg;
    size_type iend;
  };

  struct interval_t
  {
    uint16_t n;
    uint16_t m;
  };

  struct elem_t
  {
    regex_state state;
    union U
    {
      open_t open;
      close_t close;
      single_t single;
      bracket_t bracket;
      interval_t interval;
      alternation_t alternation;
    } data;

    elem_t(regex_state st)
    : state(st), data()
    {}
  };

  // TODO char_int or size_type
  using param_type = uint32_t;

  std::vector<char_int> bracket_list;
  std::vector<size_type> igroups;
  std::vector<elem_t> elems;
};

scanner_ctx scan(char const * s);

} }
