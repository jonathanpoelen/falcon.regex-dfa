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
#include <cstdint>


namespace falcon { namespace regex {

using char_int = uint32_t;
using size_type = std::size_t;

namespace detail { namespace {
  enum _regex_state_quanti {
    none,
    closure0,
    closure1,
    option,
    brace0, // {,m}
    repetition,
    brace1, // {n,}
    interval, // {n,m}
  };
} }

enum class regex_state
{
  start,
  bol,
  eol,
  open,

  single1,
  single1_closure0,
  single1_closure1,
  single1_option,
  single1_brace0, // {,m}
  single1_repetition,
  single1_brace1, // {n,}
  single1_interval, // {n,m}

  escaped,
  escaped_closure0,
  escaped_closure1,
  escaped_option,
  escaped_brace0, // {,m}
  escaped_repetition,
  escaped_brace1, // {n,}
  escaped_interval, // {n,m}

  any,
  any_closure0,
  any_closure1,
  any_option,
  any_brace0, // {,m}
  any_repetition,
  any_brace1, // {n,}
  any_interval, // {n,m}

  bracket,
  bracket_closure0,
  bracket_closure1,
  bracket_option,
  bracket_brace0, // {,m}
  bracket_repetition,
  bracket_brace1, // {n,}
  bracket_interval, // {n,m}

  bracket_reverse,
  bracket_reverse_closure0,
  bracket_reverse_closure1,
  bracket_reverse_option,
  bracket_reverse_brace0, // {,m}
  bracket_reverse_repetition,
  bracket_reverse_brace1, // {n,}
  bracket_reverse_interval, // {n,m}

  close,
  close_closure0,
  close_closure1,
  close_option,
  close_brace0, // {,m}
  close_repetition,
  close_brace1, // {n,}
  close_interval, // {n,m}

  NB
};

struct scanner_ctx
{
  struct elem_t
  {
    regex_state state;
    // TODO char_int or size_type
    uint32_t idx_or_ch;
  };

  struct alternations_t
  {
    size_type ibeg;
    size_type iend;
  };

  std::vector<alternations_t> alternations_list;
  std::vector<size_type> idx_opener_list;
  // TODO char_int or size_type
  std::vector<uint32_t> params;
  std::vector<elem_t> elems;
};

scanner_ctx scan(char const * s);

} }
