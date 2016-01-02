project(falcon.regex-dfa)
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


set(
  SRC_SCAN 
  ${SRC}/scan.cpp
  ${SRC}/print_automaton.cpp
  ${SRC}/reverse_transitions.cpp
  ${SRC}/scan_intervals.cpp
)

add_library(lib_scan ${SRC_SCAN})


link_libraries(lib_scan)


add_executable(
  re_scan
  utils/scan.cpp
)


add_executable(
  test_scan
  test/test_scan.cpp
)


add_test(re_test test_scan)

enable_testing()
