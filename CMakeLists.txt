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
if (DEFINED RE_TRACE)
  add_definitions(-DFALCON_REGEX_DFA_ENABLE_TRACE)
endif()

include_directories(include/)

# aux_source_directory(. SRC_LIST)
# file(GLOB headers_SRC "*.hpp")

set(SRC falcon/regex_dfa/)


set(
  SRC_RE_PRINT
  ${SRC}/print_automaton.cpp
  ${SRC}/print_automaton.hpp
)
add_library(lib_print ${SRC_RE_PRINT})
# link_libraries(lib_print)

set(
  SRC_RE_SCAN
  ${SRC}/scan.cpp
  ${SRC}/scan.hpp
  ${SRC}/reverse_transitions.cpp
  ${SRC}/reverse_transitions.hpp
  ${SRC}/scan_intervals.cpp
  ${SRC}/scan_intervals.hpp
)
add_library(lib_scan ${SRC_RE_SCAN})

# set(
#   SRC_RE_SCAN2
#   ${SRC}/scan2.cpp
#   ${SRC}/scan.hpp
# )
# add_library(lib_scan2 ${SRC_RE_SCAN2})

set(
  SRC_RE_MATCH
  ${SRC}/match.cpp
  ${SRC}/match.hpp
)
add_library(lib_match ${SRC_RE_MATCH})

set(
  SRC_RE_REDUCE
  ${SRC}/reduce_rng.cpp
  ${SRC}/reduce_rng.hpp
)
add_library(lib_reduce ${SRC_RE_REDUCE})

# source_group(libs FILES ${SRC_SCAN})


# Utilities
add_executable(re_match utils/match.cpp)
add_executable(re_scan utils/scan.cpp)
add_executable(new_scan utils/scan2.cpp ${SRC}/scan3.cpp)
# add_executable(re_scan2 utils/scan2.cpp)
# add_executable(re_scan_reduce utils/scan_reduce.cpp)

# target_link_libraries(re_scan2 lib_scan2)


# Tests
function(add_executable_test name)
  add_executable(test_${name} test/test_${name}.cpp)
  add_test(re_test test_${name})
endfunction()

add_executable_test(scan)
add_executable_test(match)

enable_testing()


# Links
function(link_library lib)
  foreach(exe ${ARGN})
    target_link_libraries(${exe} ${lib})
  endforeach()
endfunction()


set(EXE_SCAN re_scan test_scan)
set(EXE_MATCH re_match test_match)

link_library(lib_match ${EXE_MATCH})
link_library(lib_scan ${EXE_SCAN} ${EXE_MATCH})
link_library(lib_print ${EXE_SCAN} ${EXE_MATCH} new_scan)
