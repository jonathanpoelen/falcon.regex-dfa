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

  template<unsigned n>
  auto inc_state(falcon::regex::scanner_ctx & sctx)
  { return [&sctx](auto &) { inc_enum(sctx.elems.back().state, n); }; }
}

falcon::regex::scanner_ctx
falcon::regex::scan(char const * s)
{
  falcon::regex::scanner_ctx sctx;
  sctx.elems.push_back({regex_state::start, 0});
  sctx.alternations_list.push_back({0, 0});

  using x3::_attr;
  using x3::_pass;
//   using x3::attr;
//   using x3::phrase_parse;
//   using x3::ascii::space;

  using param_type = uint32_t;

#define SSt(name) \
  auto name = [&sctx](auto &) { sctx.elems.push_back({regex_state::name, 0}); }

#define SSta(name)                  \
  auto name = [&sctx](auto & ctx) { \
    sctx.elems.push_back({          \
      regex_state::name,            \
      param_type(_attr(ctx))        \
    });                             \
  }

  SSta(single1);

  SSt(bol);
  SSt(eol);
  SSt(any);

  auto repetition = [&sctx](auto & ctx) {
    inc_enum(sctx.elems.back().state, detail::repetition);
    sctx.params.push_back(_attr(ctx));
  };

  auto interval = [&sctx](auto & ctx) {
    inc_enum(sctx.elems.back().state, 1);
    sctx.params.push_back(_attr(ctx));
  };

  param_type bracket_pos;

  auto bracket_open = [&sctx, &bracket_pos](auto &) {
    bracket_pos = sctx.params.size();
    sctx.elems.push_back({regex_state::bracket, bracket_pos});
  };

  auto bracket_close = [&sctx, &bracket_pos](auto &) {
    sctx.params[bracket_pos] = sctx.params.size();
  };

  auto to_bracket_reverse = [&sctx](auto &) {
    sctx.elems.back().state = regex_state::bracket_reverse;
  };

  auto add_bracket_char = [&sctx](auto & ctx) {
    sctx.params.push_back(_attr(ctx));
    sctx.params.push_back(_attr(ctx));
  };

  auto add_bracket_range = [&sctx](auto & ctx) {
    sctx.params.back() = _attr(ctx);
  };


  auto escaped = [&sctx](auto & ctx) {
    switch (_attr(ctx)) {
      case 'w':
      case 'd':
      case 's':
      // TODO
        sctx.elems.push_back({regex_state::escaped, param_type(_attr(ctx))});
        break;
      default:
        sctx.elems.push_back({regex_state::single1, param_type(_attr(ctx))});
        break;
    }
  };


  auto open = [&sctx](auto &) {
    sctx.idx_opener_list.push_back(sctx.elems.size());
    sctx.elems.push_back({regex_state::open, param_type(sctx.params.size())});
    sctx.params.push_back(0);
  };

  auto close = [&sctx](auto & ctx) {
    if (sctx.idx_opener_list.empty()) {
      _pass(ctx) = false;
      return ;
    }
    sctx.elems.push_back({regex_state::close, param_type(sctx.params.size())});
    auto const i = sctx.idx_opener_list.back();
    sctx.idx_opener_list.pop_back();
    sctx.params[sctx.elems[i].idx_or_ch] = sctx.elems.size();
    sctx.params.push_back(i);
  };

  constexpr auto C = x3::ascii::char_;
  constexpr auto u16 = boost::spirit::x3::uint16;

  auto quanti_parser
  = C('*')[inc_state<detail::closure0>(sctx)]
  | C('+')[inc_state<detail::closure1>(sctx)]
  | C('?')[inc_state<detail::option>(sctx)]
  | (C('{') >>
      (
        (u16[repetition] >> C(',')[inc_state<1>(sctx)] >> -u16[interval])
      | (C(',') >> u16[inc_state<detail::brace0>(sctx)])
      )
    >> C('}'))
  ;

  auto mk_bracket_char_parser = [](auto a) {
    constexpr auto C = x3::ascii::char_;
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
  | (C('(')[open] >> -(C('?') >> (C(':') | C('!')))/*[TODO]*/)
  | ( (
        (C('\\') >> C[escaped])
      | (C('.')[any])
      | (C(')')[close])
      | (bracket_parser)
      | (C - (C('-') | C('+') | C('?') | C('{')))[single1]
      )
      >> -quanti_parser
    )
  );

  // TODO check error
  x3::parse(s, s+strlen(s), parser);

  return sctx;
}
