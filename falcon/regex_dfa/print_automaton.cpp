#include "range_iterator.hpp"
#include "print_automaton.hpp"
#include "redfa.hpp"

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <cassert>

namespace {
  constexpr char const * colors[]{
    "\033[31;02m",
    "\033[32;02m",
    "\033[33;02m",
    "\033[34;02m",
    "\033[35;02m",
    "\033[36;02m",
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
  return os
    << colors[1]
    << " [" << t.e.l << "-" << t.e.r << "]"
       " ['" << char(t.e.l) << "'-'" << char(t.e.r) << "']"
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
    << (states & Range::Final ? " @" : "  ")
    << (states & Range::Begin ? " ^" : "  ")
    << (states & Range::End ? " $" : "  ")
    << (states & Range::Normal ? " =" : "  ")
    << (states & Range::Invalid ? " x" : "  ")
    //<< (rng.states & Range::Epsilon ? "E " : "  ")
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

void print_automaton(const Ranges& rngs)
{
  auto sz = rngs.size() * 2u;
  for (auto & rng : rngs) {
    sz += rng.transitions.size();
  }
  
  struct L { std::size_t n; enum { None, Start, Plug, Stop, Cont } e; };
  using Graph = std::vector<std::vector<L>>;
  
  auto process_graph = [sz, &rngs](Graph & graph, auto next_filter_op, auto f_ok, auto process_line){
    auto add_column = [sz, &graph](){
      graph.emplace_back();
      graph.back().resize(sz, L{~std::size_t{}, L::None});
    };
    add_column();
    
    unsigned i = 1;
    auto first_rng = begin(rngs);
    auto last_rng = end(rngs);
    for (; first_rng != last_rng; ++first_rng) {
      auto rng_num = std::size_t(first_rng - begin(rngs));
      
      auto first_trs = begin(first_rng->transitions);
      auto last_trs = end(first_rng->transitions);
      for (; first_trs != last_trs; ++first_trs, ++i) {
        auto next = first_trs->next;
        if (next_filter_op(rng_num, next)) {
          continue;
        }
        auto pgraph = std::find_if(begin(graph), end(graph), [i, next](auto & v){
          return v[i].n == next;
        });
        if (end(graph) == pgraph) {
          auto p = begin(graph);
          do {
            p = std::find_if(p, end(graph), [i](auto & v){
              return v[i].e == L::None;
            });
          } while (p != end(graph) && !f_ok(&(*p)[i], first_rng, first_trs) && (++p, 1));
          if (end(graph) == p) {
            add_column();
            p = end(graph)-1;
          }
          
          process_line(&(*p)[i], first_rng, first_trs);
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
    graph, std::greater_equal<>{}, 
    [](auto, auto, auto){ return std::true_type{}; }, 
    [&rngs](auto p, auto it_rng, auto it_trs){
      auto next = it_trs->next;
      std::size_t n = end(it_rng->transitions) - it_trs;
      auto first_rng2 = it_rng;
      auto last_rng2 = begin(rngs) + next;
      while (++first_rng2 != last_rng2) {
        n += first_rng2->transitions.size() + 2;
      }
      
      *p = L{next, L::Start};
      *std::fill_n(++p, n, L{next, L::Cont}) = L{next, L::Stop};
    }
  );
  Graph graph2;
  bool empty_graph2 = true;
  process_graph(
    graph2, std::less<>{}, 
    [&rngs](auto p, auto it_rng, auto it_trs){
      p -= it_trs - begin(it_rng->transitions) + 1;
      auto first_rng2 = begin(rngs) + it_trs->next;
      for (; first_rng2 != it_rng; ++first_rng2) {
        if (p->e != L::None) {
          return false;
        }
        p -= first_rng2->transitions.size() + 2;
      }
      return true;
    }, 
    [&rngs, &empty_graph2](auto p, auto it_rng, auto it_trs){
      auto next = it_trs->next;
      std::size_t n = it_trs - begin(it_rng->transitions) + 1;
      auto first_rng2 = begin(rngs) + next;
      for (; first_rng2 != it_rng; ++first_rng2) {
        n += first_rng2->transitions.size() + 2;
      }
      
      *p = L{next, L::Stop};
      p -= n;
      *p = L{next, L::Start};
      std::fill_n(++p, n-1, L{next, L::Cont});
      empty_graph2 = false;
    }
  );

  auto igraph = 0;
  auto print_tree =[&igraph, &graph, &graph2, empty_graph2]{
    auto print_graph = [&igraph](Graph const & graph, auto ecmp) {
      constexpr char const * borders[]{"  ", "┌─", "├─", "└─", "│ "};

      unsigned i = 0;
      unsigned pluged_color = nb_colors;
      bool is_stop = false;
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
          is_stop = (ecmp == e);
          ++first;
          break;
        }
        ++i;
      }
      
      constexpr char const * borders2[]{"", "│", "│", "┴", "│"};
      
      for (; first != last; ++first) {
        ++i;
        auto const e = (*first)[igraph].e;
        if (e) {
          std::cout << colors[i % nb_colors] << borders2[e] << colors[pluged_color] << "─";
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
    
    if (!empty_graph2) {
      print_graph(graph2, L::Start);
      std::cout << reset_color << "┆";      
    }
    print_graph(graph, L::Stop);
    
    ++igraph;
  };
  
  for (auto & rng : rngs) {
    print_tree();
    print_head_rng(rng, int(&rng-&rngs[0]));
    std::cout << "\n";
    for (auto & t : rng.transitions) {
      print_tree();
      std::cout << t << "\n";
    }
    print_tree();
    std::cout << "\n";
  }

  print_table(rngs.capture_table);
}

} }