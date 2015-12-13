project(regex)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)

# find_program(CCACHE_FOUND ccache)
# if(CCACHE_FOUND)
#   set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
#   set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
# endif(CCACHE_FOUND)

include(CMakeDefinitions.txt)
include_directories(.)
add_definitions(
  -g
  -fsanitize=address
)
link_libraries(-lasan)

# file(GLOB headers_SRC "*.hpp")
include_directories(.)

set(SRC falcon/regex_dfa/)

add_executable(
  test_scan
  test/test_scan.cpp
  ${SRC}/scan.cpp
  ${SRC}/print_automaton.cpp
  ${SRC}/reverse_transitions.cpp
  ${SRC}/scan_intervals.cpp
)

add_test(regex_test test_scan)

enable_testing()
