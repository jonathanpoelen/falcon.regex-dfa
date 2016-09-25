#include "falcon/regex_dfa/scanner/match.hpp"
#include "falcon/regex_dfa/scanner/scanner_io.hpp"
#include "falcon/regex_dfa/regex_consumer.hpp"
#include "falcon/regex_dfa/range_iterator.hpp"

#include <iostream>
#include <functional>
#include <cassert>

namespace falcon { namespace regex {

namespace {

scanner_ctx::elem_t const & next(scanner_ctx::elem_t const & e)
{ return *(&e + 1); }

scanner_ctx::elem_t const & next(scanner_ctx::elem_t const * e)
{ return *(e + 1); }

scanner_ctx::elem_t const * pnext(scanner_ctx::elem_t const & e)
{ return &e + 1; }

scanner_ctx::elem_t const * pnext(scanner_ctx::elem_t const * e)
{ return e + 1; }

}

bool match(scanner_ctx const & ctx, char const * s)
{
  regex_dfa::utf8_consumer consumer{s};

  auto const bbeg = ctx.bracket_list.begin();
  auto const ebeg = ctx.elems.begin();
  auto const edata = ctx.elems.data();

  using elem_t = scanner_ctx::elem_t const;
  using ref_elem_t = std::reference_wrapper<elem_t>;
  std::vector<ref_elem_t> elems1;
  std::vector<ref_elem_t> elems2;
  std::vector<bool> used_lookup1(ctx.elems.size(), false);
  std::vector<bool> used_lookup2(ctx.elems.size(), false);

  elems1.push_back(*ebeg);
  used_lookup1[0] = true;

  auto add = [edata](
    elem_t & e,
    std::vector<bool> & used_lookup,
    std::vector<ref_elem_t> & elems
  ) {
    auto && is_enable = used_lookup[&e - edata];
    if (!is_enable) {
      elems.push_back(e);
      is_enable = true;
    }
  };

  auto add1 = [add, &used_lookup1, &elems1](elem_t & e) {
    add(e, used_lookup1, elems1);
  };

  auto add2 = [add, &used_lookup2, &elems2](elem_t & e) {
    add(e, used_lookup2, elems2);
  };

  constexpr auto unspecified_next = ~ size_type {};

  while (!elems1.empty()) {
    auto c = consumer.bumpc();
    if (!c) {
      for (elem_t & e : elems1) {
        print_elem(ctx, std::cout << " x ", e) << "\n";
        if (e.state == regex_state::terminate) {
          return true;
        }
      }
      break;
    }

    auto eit = elems1.begin();
    auto eend = elems1.end();

    auto updrng = [&elems1, &eit, &eend](size_type i) {
      eit = elems1.begin() + i;
      eend = elems1.end();
    };

    auto itpos = [&elems1, &eit]() {
      return eit - elems1.begin();
    };

    auto updit = [edata, &used_lookup1, &eit](elem_t & e) {
      auto && is_enable = used_lookup1[&e - edata];
      if (!is_enable) {
        *eit = e;
        is_enable = true;
      }
      else {
        ++eit;
      }
    };

    while (eit != eend) {
      elem_t & e = *eit;

      print_elem(ctx, std::cout, e) << "\n";

      switch (e.state) {
        case regex_state::NB:
          assert(false);
          break;

        case regex_state::terminate:
          return true;

        case regex_state::bol: {
          regex_dfa::utf8_consumer consumer2{s};
          consumer2.bumpc();
          if (consumer2.s == consumer.s) {
            updit(e);
          }
          else {
            ++eit;
          }
        } break;

        case regex_state::eol:
        case regex_state::alternation:
          ++eit;
          break;

        case regex_state::open:
          // TODO
        case regex_state::open_nocap:
        case regex_state::start: {
          elem_t * alterp = pnext(e);
          updit(next(*alterp));
          auto pos = itpos();
          while (alterp->data.alternation.inc_next != unspecified_next) {
            alterp += alterp->data.alternation.inc_next;
            add1(next(alterp));
          }
          updrng(pos);
        } break;

        case regex_state::any: {
          elem_t & quanti = next(e);
          print_elem(ctx, std::cout << " ", quanti) << "\n";
          switch (quanti.state) {
            case regex_state::brace0:
              // TODO
              break;
            case regex_state::repetition:
              // TODO
              break;
            case regex_state::brace1:
              // TODO
              break;
            case regex_state::interval:
              // TODO
              break;
            case regex_state::closure0:
              updit(next(quanti));
              add2(e);
              add2(next(quanti));
              break;
            case regex_state::closure1:
              add2(next(quanti));
              break;
            case regex_state::option:
              updit(next(quanti));
              add2(next(quanti));
              break;
            default :
              add2(quanti);
              ++eit;
          }
        } break;


        // TODO

        case regex_state::brace0:
          break;
        case regex_state::repetition:
          break;
        case regex_state::brace1:
          break;
        case regex_state::interval:
          break;
        case regex_state::closure0: break;
        case regex_state::closure1: break;
        case regex_state::option: break;

        case regex_state::close:
          break;

        case regex_state::single1:
        case regex_state::escaped:
          break;

        case regex_state::bracket:
        case regex_state::bracket_reverse:
          for (auto && c : falcon::make_range(
            bbeg + e.data.bracket.ibeg,
            bbeg + e.data.bracket.iend
          )) {
          }
          break;
      }
    }

    using std::swap;
    swap(elems1, elems2);
    swap(used_lookup1, used_lookup2);
    elems2.clear();
    used_lookup2.assign(used_lookup2.size(), false);
  }

  return false;
}

} }
