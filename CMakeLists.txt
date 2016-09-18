project(falcon.regex-dfa)
cmake_minimum_required(VERSION 2.8)


include(CMakeDefinitions.txt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -iquote ${PROJECT_SOURCE_DIR}/.")

# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
# set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (DEFINED SAN)
  SET(SAN_FLAGS "
    -fsanitize=address
    -fsanitize=undefined
  ")
  add_definitions(${SAN_FLAGS})
  SET(LINK_FLAGS "${LINK_FLAGS} ${SAN_FLAGS}")
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


# set(
#   SRC_RE_PRINT
#   ${SRC}/print_automaton.cpp
#   ${SRC}/print_automaton.hpp
# )
# add_library(re_print ${SRC_RE_PRINT})
# # link_libraries(re_print)
#
# set(
#   SRC_RE_SCAN3
#   ${SRC}/scan3.cpp
#   ${SRC}/scan.hpp
# )
# add_library(re_scan3 ${SRC_RE_SCAN3})
#
# set(
#   SRC_RE_MATCH
#   ${SRC}/match.cpp
#   ${SRC}/match.hpp
# )
# add_library(re_match ${SRC_RE_MATCH})

set(
  SRC_RE_SCANNER
  ${SRC}/scanner/scanner.cpp
  ${SRC}/scanner/scanner.hpp
)
add_library(re_scanner ${SRC_RE_SCANNER})

# source_group(libs FILES ${SRC_SCAN})


# Utilities
# add_executable(match utils/match.cpp)
# add_executable(re_scan utils/scan.cpp)
# add_executable(new_scan utils/scan2.cpp)
add_executable(scanner utils/scanner.cpp)
# add_executable(re_scan2 utils/scan2.cpp)
# add_executable(re_scan_reduce utils/scan_reduce.cpp)

# target_link_libraries(re_scan2 lib_scan2)


# Tests
function(add_test_executable name)
  add_executable(test_${name} test/test_${name}.cpp)
  add_test(re_test test_${name})
endfunction()

# add_test_executable(scan)
# add_test_executable(scan3)
# add_test_executable(match)

enable_testing()


# Links
function(link_library lib)
  foreach(exe ${ARGN})
    target_link_libraries(${exe} ${lib})
  endforeach()
endfunction()


# set(EXE_SCAN re_scan test_scan)
# set(EXE_SCAN3 new_scan test_scan3 match)
set(EXE_SCANNER scanner)
# set(EXE_MATCH match)
# set(EXE_MATCH re_match test_match)

# link_library(re_match ${EXE_MATCH})
# link_library(lib_scan ${EXE_SCAN} ${EXE_MATCH})
# link_library(re_print ${EXE_SCAN} ${EXE_MATCH} new_scan test_scan3)
# link_library(re_scan3 ${EXE_SCAN3})
# link_library(re_print ${EXE_SCAN3})
# link_library(re_match ${EXE_MATCH})
link_library(re_scanner ${EXE_SCANNER})
