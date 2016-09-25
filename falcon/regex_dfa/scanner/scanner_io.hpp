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

#include "falcon/regex_dfa/scanner/scanner.hpp"
#include "falcon/regex_dfa/range_iterator.hpp"


namespace falcon { namespace regex {

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> &
print_elem(
  scanner_ctx const & ctx,
  std::basic_ostream<Ch, Tr> & out,
  scanner_ctx::elem_t const & e
) {
  out << e.state;
  switch (e.state) {
    case regex_state::NB: break;
    case regex_state::bol: break;
    case regex_state::eol: break;
    case regex_state::terminate: break;
    case regex_state::start: break;
    case regex_state::any: break;

    case regex_state::brace0:
      out << "  {," << e.data.interval.m << "}";
      break;
    case regex_state::repetition:
      out << "  {" << e.data.interval.n << "}";
      break;
    case regex_state::brace1:
      out << "  {" << e.data.interval.n << ",}";
      break;
    case regex_state::interval:
      out << "  {" << e.data.interval.n << "," << e.data.interval.m << "}";
      break;
    case regex_state::closure0: break;
    case regex_state::closure1: break;
    case regex_state::option: break;

    case regex_state::alternation:
      out << "  inc_next: +" << e.data.alternation.inc_next;
      break;

    case regex_state::open:
    case regex_state::open_nocap:
      out << "  inc_close: +" << e.data.open.inc_close;
      out << "  inc_last_alternation: +" << e.data.open.inc_last_alternation;
      break;

    case regex_state::close:
      out << "  dec_open: -" << e.data.close.dec_open;
      break;

    case regex_state::single1:
    case regex_state::escaped:
      out << "  " << char(e.data.single.c);
      break;

    case regex_state::bracket:
    case regex_state::bracket_reverse:
      out << "  [";
      for (auto && c : falcon::make_range(
        ctx.bracket_list.begin() + e.data.bracket.ibeg,
        ctx.bracket_list.begin() + e.data.bracket.iend
      )) {
        out << char(c);
      }
      out << "]";
      break;
  }
  return out;
}

template<class Ch, class Tr>
std::basic_ostream<Ch, Tr> &
operator<<(std::basic_ostream<Ch, Tr> & out, scanner_ctx const & ctx)
{
  unsigned n = 0;

  for (scanner_ctx::elem_t const & e : ctx.elems) {
    out.width(3);
    print_elem(ctx, out << n++ << " ", e) << "\n";
  }
  return out << "\n";
}

} }
