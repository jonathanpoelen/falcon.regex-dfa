project(falcon.regex-dfa)
cmake_minimum_required(VERSION 2.8)


include(CMakeDefinitions.txt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -iquote ${PROJECT_SOURCE_DIR}/.")

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (DEFINED ASAN)
  add_definitions(-fsanitize=address)
  link_libraries(-lasan)
endif()
if (DEFINED DEBUG_FULL)
  add_definitions(-D_GLIBCXX_DEBUG)
endif()


# aux_source_directory(. SRC_LIST)
# file(GLOB headers_SRC "*.hpp")

set(SRC falcon/regex_dfa/)


set(
  SRC_SCAN 
  ${SRC}/scan.cpp
  ${SRC}/scan.hpp
  ${SRC}/print_automaton.cpp
  ${SRC}/print_automaton.hpp
  ${SRC}/reverse_transitions.cpp
  ${SRC}/reverse_transitions.hpp
  ${SRC}/scan_intervals.cpp
  ${SRC}/scan_intervals.hpp
)
add_library(lib_scan ${SRC_SCAN})
link_libraries(lib_scan)

# source_group(libs FILES ${SRC_SCAN})

add_executable(
  re_scan
  utils/scan.cpp
)

add_executable(
  test_scan
  test/test_scan.cpp
)
# target_link_libraries(test_scan lib_scan)


add_test(re_test test_scan)

enable_testing()
