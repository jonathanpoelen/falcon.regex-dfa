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
  { return [&sctx](auto &) { inc_enum(sctx.elems.back(), n); }; }
}

falcon::regex::scanner_ctx
falcon::regex::scan(char const * s)
{
  falcon::regex::scanner_ctx sctx;
  sctx.elems.emplace_back(regex_state::start);
  // first alternation
  sctx.params.emplace_back();
  // last alternation
  sctx.params.emplace_back();

  using x3::_attr;
  using x3::_pass;

  using param_type = falcon::regex::scanner_ctx::param_type;

#define SSt(name) \
  auto name = [&sctx](auto &) { sctx.elems.emplace_back(regex_state::name); }

  SSt(bol);
  SSt(eol);
  SSt(any);

  auto single1 = [&sctx](auto & ctx) {
    sctx.elems.emplace_back(regex_state::single1);
    sctx.params.emplace_back(param_type(_attr(ctx)));
  };

  auto repetition = [&sctx](auto & ctx) {
    inc_enum(sctx.elems.back(), detail::repetition);
    sctx.params.emplace_back(_attr(ctx));
  };

  auto interval = [&sctx](auto & ctx) {
    inc_enum(sctx.elems.back(), 1);
    sctx.params.emplace_back(_attr(ctx));
  };

  param_type bracket_pos;

  auto bracket_open = [&sctx, &bracket_pos](auto &) {
    bracket_pos = param_type(sctx.params.size());
    sctx.elems.emplace_back(regex_state::bracket);
    sctx.params.emplace_back();
  };

  auto bracket_close = [&sctx, &bracket_pos](auto &) {
    sctx.params[bracket_pos] = param_type(sctx.params.size());
  };

  auto to_bracket_reverse = [&sctx](auto &) {
    sctx.elems.back() = regex_state::bracket_reverse;
  };

  auto add_bracket_char = [&sctx](auto & ctx) {
    sctx.params.emplace_back(_attr(ctx));
    sctx.params.emplace_back(_attr(ctx));
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
        sctx.elems.emplace_back(regex_state::escaped);
        break;
      default:
        sctx.elems.emplace_back(regex_state::single1);
        break;
    }
    sctx.params.emplace_back(_attr(ctx));
  };

  param_type first_altern = 0;

  auto open = [&sctx, &first_altern](auto &) {
    sctx.stack_params.emplace_back(first_altern);
    sctx.stack_params.emplace_back(sctx.elems.size());
    sctx.stack_params.emplace_back(sctx.params.size());
    first_altern = param_type(sctx.stack_params.size());
    sctx.elems.emplace_back(regex_state::open);
    // idx close
    sctx.params.emplace_back();
    // first alternation
    sctx.params.emplace_back();
    // last alternation
    sctx.params.emplace_back();
  };

  auto close = [&sctx, &first_altern](auto & ctx) {
    if (first_altern == 0) {
      _pass(ctx) = false;
      return ;
    }
    auto const iparam = sctx.stack_params[first_altern - 1u];
    auto const ielem = sctx.stack_params[first_altern - 2u];
    // idx close
    sctx.params[iparam] = param_type(sctx.elems.size());
    // idx open
    sctx.params.emplace_back(ielem);
    sctx.params.emplace_back(iparam);
    sctx.params[iparam + 1u] = param_type(sctx.params.size());
    sctx.elems.emplace_back(regex_state::close);

    param_type previous_first_altern = sctx.stack_params[first_altern - 3u];
    if (first_altern != sctx.stack_params.size()) {
      inc_enum(sctx.elems[ielem], 1);
      sctx.params.insert(
        sctx.params.end(),
        sctx.stack_params.begin() + first_altern,
        sctx.stack_params.end()
      );
    }

    sctx.params[iparam + 2u] = param_type(sctx.params.size());
    sctx.stack_params.resize(first_altern - 3u);

    first_altern = previous_first_altern;
  };

  auto alternation = [&sctx](auto &) {
    sctx.stack_params.emplace_back(sctx.elems.size() + 1);
    sctx.elems.emplace_back(regex_state::alternation);
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
      | (C(',') >> u16[inc_state<detail::brace0>(sctx)])
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
      | (C(')')[close])
      | (bracket_parser)
      | -(C('*') | C('+') | C('?') | C('{')) >> C[single1]
      )
      >> -quanti_parser
    )
  );

  auto send = s+strlen(s);
  auto res = x3::parse(s, send, parser);
  if (!res || s != send || first_altern) {
    sctx = {};
  }
  else {
    if (sctx.stack_params.size()) {
      inc_enum(sctx.elems[0], 1);
      sctx.params[0] = param_type(sctx.params.size());
      sctx.params[1] = param_type(sctx.params.size() + sctx.stack_params.size());
      sctx.params.insert(
        sctx.params.end(),
        sctx.stack_params.begin(),
        sctx.stack_params.end()
      );
    }
    sctx.elems.push_back(regex_state::terminate);
  }

  return sctx;
}
