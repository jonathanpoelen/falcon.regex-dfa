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

#include "falcon/regex_dfa/scanner/scanner.hpp"

#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

namespace
{
  template<class T>
  constexpr std::underlying_type_t<T>
  underlying_cast(T e) noexcept
  { return static_cast<std::underlying_type_t<T>>(e); }

  template<class T>
  void inc_enum(T & e, unsigned n) noexcept
  { e = static_cast<T>(underlying_cast(e) + n); }

  using Ctx = falcon::regex::scanner_ctx;

  template<unsigned n>
  auto inc_state(Ctx & sctx)
  { return [&sctx](auto &) { inc_enum(sctx.elems.back().state, n); }; }

  Ctx::elem_t & push_elem(Ctx & sctx, falcon::regex::regex_state st)
  {
    sctx.elems.emplace_back(st);
    return sctx.elems.back();
  }

  Ctx::elem_t & back_elem(Ctx & sctx)
  {
    return sctx.elems.back();
  }

  Ctx::elem_t & elem(Ctx & sctx, unsigned i)
  {
    return sctx.elems[i];
  }
}

namespace falcon { namespace regex {

scanner_ctx scan(char const * s)
{
  falcon::regex::scanner_ctx sctx;
  push_elem(sctx, regex_state::start)
  .data.start = {};

  using x3::_attr;
  using x3::_pass;
  using elem_t = scanner_ctx::elem_t;

#define SSt(name) \
  auto name = [&sctx](auto &) { sctx.elems.emplace_back(regex_state::name); }

  SSt(bol);
  SSt(eol);
  SSt(any);

  auto single1 = [&sctx](auto & ctx) {
    push_elem(sctx, regex_state::single1)
    .data.single.c = _attr(ctx);
  };

  auto brace0 = [&sctx](auto & ctx) {
    push_elem(sctx, regex_state::brace0)
      .data.interval.m = _attr(ctx);
  };

  auto repetition = [&sctx](auto & ctx) {
    push_elem(sctx, regex_state::repetition)
    .data.interval.n = _attr(ctx);
  };

  auto interval = [&sctx](auto & ctx) {
    elem_t & e = back_elem(sctx);
    inc_enum(e.state, 1);
    e.data.interval.m = _attr(ctx);
  };

  unsigned bracket_pos;

  auto bracket_open = [&sctx, &bracket_pos](auto &) {
    bracket_pos = unsigned(sctx.bracket_list.size());
    push_elem(sctx, regex_state::bracket);
  };

  auto bracket_close = [&sctx, &bracket_pos](auto &) {
    back_elem(sctx).data.bracket = {
      bracket_pos, unsigned(sctx.bracket_list.size())
    };
  };

  auto to_bracket_reverse = [&sctx](auto &) {
    back_elem(sctx).state = regex_state::bracket_reverse;
  };

  auto add_bracket_char = [&sctx](auto & ctx) {
    sctx.bracket_list.emplace_back(_attr(ctx));
    sctx.bracket_list.emplace_back(_attr(ctx));
  };

  auto add_bracket_range = [&sctx](auto & ctx) {
    sctx.bracket_list.back() = _attr(ctx);
  };


  auto escaped = [&sctx](auto & ctx) {
    switch (_attr(ctx)) {
      case 'w':
      case 'd':
      case 's':
      // TODO
        push_elem(sctx, regex_state::escaped)
        .data.single.c = _attr(ctx);
        break;
      default:
        push_elem(sctx, regex_state::single1)
        .data.single.c = _attr(ctx);
        break;
    }
  };

  auto open = [&sctx](auto &) {
    sctx.stack_params.emplace_back(sctx.elems.size());
    push_elem(sctx, regex_state::open)
    .data.open.ibeg = unsigned(sctx.stack_alter_list.size());
  };

//   auto check_close = [&sctx](auto & ctx) {
//     if (sctx.alter_list.size()) {
//       _pass(ctx) = false;
//     }
//   };

  auto close = [&sctx](auto &) {
    // TODO check
    auto const ielem = sctx.stack_params.back();
    sctx.stack_params.pop_back();
    auto & open_data = elem(sctx, ielem).data.open;
    auto stack_ibeg = open_data.ibeg;
    open_data.ibeg = unsigned(sctx.alter_list.size());
    sctx.alter_list.insert(
      sctx.alter_list.end(),
      sctx.stack_alter_list.begin() + stack_ibeg,
      sctx.stack_alter_list.end()
    );
    sctx.stack_alter_list.resize(open_data.ibeg);
    open_data.iend = unsigned(sctx.alter_list.size());
    open_data.idx_close = unsigned(sctx.elems.size());
    push_elem(sctx, regex_state::close)
    .data.close = {ielem};
  };

  auto alternation = [&sctx](auto &) {
    sctx.stack_alter_list.emplace_back(sctx.elems.size() + 1);
    push_elem(sctx, regex_state::alternation);
  };


  auto const C = x3::ascii::char_;
  auto const u16 = boost::spirit::x3::uint16;

  auto quanti_parser
  = C('*')[inc_state<detail::closure0>(sctx)]
  | C('+')[inc_state<detail::closure1>(sctx)]
  | C('?')[inc_state<detail::option>(sctx)]
  | (C('{') >>
      (
        (u16[repetition] >> C(',')[inc_state<1>(sctx)] >> -u16[interval])
      | (C(',') >> u16[brace0])
      )
    >> C('}'))
  ;

  auto mk_bracket_char_parser = [](auto a) {
    auto const C = x3::ascii::char_;
    return (
      (C('\'') >> C[a])
    | (C - C(']'))[a]
    );
  };

  auto bracket_parser
   = C('[')[bracket_open]
  >> -C('^')[to_bracket_reverse]
  >> -C('-')[add_bracket_char]
  >> *(
    mk_bracket_char_parser(add_bracket_char)
    >> -(C('-') >> mk_bracket_char_parser(add_bracket_range))
  )
  >> C(']')[bracket_close];

  auto parser = *
  ( C('^')[bol]
  | C('$')[eol]
  | C('|')[alternation]
  | (C('(')[open] >> -(C('?') >> (C('!')[inc_state<1>(sctx)])))
  | ( (
        (C('\\') >> C[escaped])
      | (C('.')[any])
      | (C(')')/* >> x3::attr(0)[check_close] >> x3::attr(0)*/[close])
      | (bracket_parser)
      | (-(C('*') | C('+') | C('?') | C('{')) >> C[single1])
      )
      >> -quanti_parser
    )
  );

  auto send = s+strlen(s);
  auto res = x3::parse(s, send, parser);
  if (!res || s != send || sctx.stack_params.size()) {
    //sctx = {};
  }
  else {
    if (sctx.stack_alter_list.size()) {
      elem_t & e = sctx.elems[0];
      inc_enum(e.state, 1);
      e.data.start.ibeg = unsigned(sctx.alter_list.size());
      sctx.alter_list.insert(
        sctx.alter_list.end(),
        sctx.stack_alter_list.begin(),
        sctx.stack_alter_list.end()
      );
      e.data.start.iend = unsigned(sctx.alter_list.size());
    }
    sctx.elems.push_back(regex_state::terminate);
  }

  return sctx;
}

} }
