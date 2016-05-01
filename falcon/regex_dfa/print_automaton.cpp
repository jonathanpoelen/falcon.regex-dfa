#include "range_iterator.hpp"
#include "print_automaton.hpp"
#include "redfa.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace {
  constexpr char const * colors[]{
    "\033[31;2m",
    "\033[32;2m",
    "\033[33;2m",
    "\033[34;2m",
    "\033[35;2m",
    "\033[36;2m",
  };
  constexpr std::size_t nb_colors = sizeof(colors)/sizeof(colors[0]);
  constexpr char const * reset_color = "\033[0m";

  template<class T>
  struct SetColor { T const & x; unsigned i; };

  template<class T>
  SetColor<T> setcolor(T const & x, unsigned i)
  {
    assert(i < nb_colors);
    return {x, i};
  }

  template<class T>
  std::ostream & operator<<(std::ostream & os, SetColor<T> x)
  { return os << colors[x.i] << x.x << reset_color; }
}

namespace falcon { namespace regex_dfa {

std::ostream & operator<<(std::ostream & os, Transition const & t)
{
  os
    << colors[1]
    << "[" << t.e.l << "-" << t.e.r << "]"
       " ['" << char(t.e.l) << "'-'" << char(t.e.r) << "']"
  ;
  if (t.states) {
    os << colors[4]
      << (t.states & Transition::Bol    ? " ^" : "  ")
      << (t.states & Transition::Normal ? " =" : "  ")
    ;
  }
  return os
    << reset_color
    << " → " << t.next
  ;
}

std::ostream & operator<<(std::ostream & os, Transitions const & ts)
{
  for (auto & t : ts) {
    os << t << "\n";
  }
  return os << "\n";
}

std::ostream & operator<<(std::ostream & os, Range::State const & states)
{
  return os
    << (states & Range::Final   ? " @" : "  ")
    << (states & Range::Bol     ? " ^" : "  ")
    << (states & Range::Eol     ? " $" : "  ")
    << (states & Range::Normal  ? " =" : "  ")
    << (states & Range::Invalid ? " x" : "  ")
  ;
}

std::ostream & operator<<(std::ostream & os, Capture const & capstate) {
  constexpr char const * c[]{
    "",
    "(",
    ")", "()",
    "+", "(+", "+)", "(+)",
    ".", ".(", ".)", ".()", ".+", ".(+", ".+)", ".(+)",
    "-", "-(", "-)", "-()", "-+", "-(+", "-+)", "-(+)",
      "-.", "-.(", "-.)", "-.()", "-.+", "-.(+", "-.+)", "-.(+)",
  };
  static_assert(sizeof(c)/sizeof(c[0]) == Capture::State::MAX, "");
  return os << c[capstate.e] << capstate.n << " ";
}

void print_head_rng(const Range& rng, int num)
{
  std::cout
    << std::setw(4) << num
    << colors[4]
    << rng.states
    << colors[3]
  ;
  for (auto & capstate : rng.capstates) {
    std::cout << capstate;
  }
  std::cout << reset_color;
}

void print_automaton(const Range& rng, int num)
{
  print_head_rng(rng, num);
  std::cout << "\n" << rng.transitions;
}

void print_table(std::vector<unsigned> const & capture_table)
{
  for (auto & caps : capture_table) {
    constexpr char c[]{'(',')'};
    std::cout << caps << " " << c[capture_table[caps] > caps] << "\n";
  }
}

// void print_automaton(const Ranges& rngs)
// {
//   for (auto & rng : rngs) {
//     print_automaton(rng, int(&rng-&rngs[0]));
//   }
//
//   for (auto & caps : rngs.capture_table) {
//     constexpr char c[]{'(',')'};
//     std::cout << caps << " " << c[rngs.capture_table[caps] > caps] << "\n";
//   }
// }

struct Begin {
  template<class Cont> static auto begin(Cont & cont) { using std::begin; return begin(cont); }
  template<class Cont> static auto end(Cont & cont) { using std::end; return end(cont); }
};

struct RBegin {
  template<class Cont> static auto begin(Cont & cont) { using std::rbegin; return rbegin(cont); }
  template<class Cont> static auto end(Cont & cont) { using std::rend; return rend(cont); }
};

void print_automaton(const Ranges& rngs)
{
  auto sz = rngs.size() * 2u;
  for (auto & rng : rngs) {
    sz += rng.transitions.size();
  }

  struct L { std::size_t n; enum { None, Start, Plug, Stop, Cont } e; };
  using Graph = std::vector<std::vector<L>>;

  auto process_graph = [sz, &rngs](Graph & graph, auto is_next_op2, auto distance_next, auto a){
    auto egraph = end(graph);
    auto add_column = [sz, &graph, &egraph, a](){
      graph.emplace_back();
      graph.back().resize(sz, L{~std::size_t{}, L::None});
      egraph = end(graph);
      return egraph-1;
    };

    unsigned i = 1;
    unsigned rng_num = 0;
    auto first_rng = a.begin(rngs);
    auto last_rng = a.end(rngs);
    for (; first_rng != last_rng; ++first_rng, ++rng_num) {
      auto first_trs = a.begin(first_rng->transitions);
      auto last_trs = a.end(first_rng->transitions);
      for (; first_trs != last_trs; ++first_trs, ++i) {
        auto next = first_trs->next;
        if (is_next_op2(rng_num, next)) {
          continue;
        }
        auto pgraph = std::find_if(begin(graph), egraph, [i, next](auto & v){
          return v[i].n == next;
        });
        if (egraph == pgraph) {
          auto dist = distance_next(first_rng, first_trs);
          assert(i + dist >= 0);
          assert(decltype(sz)(i + dist) < sz);
          pgraph = std::find_if(begin(graph), egraph, [i](auto & v){
            return v[i].e == L::None;
          });

          if (pgraph == egraph) {
            pgraph = add_column();
          }

          auto first = &(*pgraph)[i];
          auto last = first + dist;
          *first = {next, L::Start};
          *last = {next, L::Stop};
          assert(first != last);
          std::fill(++first, last, L{next, L::Cont});
        }
        else {
          (*pgraph)[i].e = L::Plug;
        }
      }
      i += 2;
    }
  };

  Graph graph;
  process_graph(
    graph,
    std::greater_equal<>{},
    [&rngs](auto it_rng, auto it_trs){
      return std::accumulate(
        it_rng+1,
        begin(rngs) + it_trs->next,
        end(it_rng->transitions) - it_trs + 1u,
        [](auto n, auto & rng) { return n + rng.transitions.size() + 2; }
      );
    },
    Begin()
  );
  Graph graph2;
  process_graph(
    graph2,
    [&rngs](unsigned rev_rng_num, auto next) {
      return rngs.size() - rev_rng_num <= next;
    },
    [&rngs](auto rit_rng, auto it_trs){
      return std::accumulate(
        begin(rngs) + it_trs->next,
        rit_rng.base() - 1,
        rend(rit_rng->transitions) - it_trs,
        [](auto n, auto & rng) { return n + rng.transitions.size() + 2; }
      );
    },
    RBegin()
  );

  auto igraph = 0;
  auto print_tree =[sz, &igraph, &graph, &graph2]{
    auto print_graph = [](Graph const & graph, auto rev, auto igraph) {
      constexpr char const * borders_nor[]{"  ", "┌─", "├─", "└─", "│ "};
      constexpr char const * borders_rev[]{"  ", "└─", "├─", "┌─", "│ "};
      auto & borders = rev ? borders_rev : borders_nor;

      auto i = graph.size() - 1u;
      auto pluged_color = nb_colors;
      auto is_stop = false;
      auto first = rbegin(graph);
      auto last = rend(graph);
      std::cout << " ";

      for (; first != last; ++first) {
        auto const e = (*first)[igraph].e;
        if (e) {
          std::cout << colors[i % nb_colors];
        }
        std::cout << borders[e];
        if (L::Start <= e && L::Stop >= e) {
          pluged_color = i % nb_colors;
          is_stop = (L::Stop == e);
          ++first;
          break;
        }
        --i;
      }

      for (; first != last; ++first) {
        --i;
        auto const e = (*first)[igraph].e;
        if (e) {
          std::cout << colors[i % nb_colors] << "│" << colors[pluged_color] << "─";
        }
        else {
          std::cout << colors[pluged_color] << "──";
        }
      }

      if (pluged_color == nb_colors) {
        std::cout << " " << reset_color;
      }
      else {
        std::cout << (is_stop ? "▸" : "─");
      }
    };

    if (!graph2.empty()) {
      print_graph(graph2, true, sz - igraph - 1);
      std::cout << reset_color << "┆";
    }
    print_graph(graph, false, igraph);

    ++igraph;
  };

  for (auto & rng : rngs) {
    print_tree();
    std::cout << " ";
    print_head_rng(rng, int(&rng-&rngs[0]));
    std::cout << "\n";
    for (auto & t : rng.transitions) {
      print_tree();
      std::cout << " " << t << "\n";
    }
    print_tree();
    std::cout << "\n";
  }

  print_table(rngs.capture_table);
}

} }
