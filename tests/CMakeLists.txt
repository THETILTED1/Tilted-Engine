# Prefer an installed GoogleTest (e.g. from the conda env); otherwise fetch it.
find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
  include(FetchContent)
  # EXCLUDE_FROM_ALL keeps GoogleTest's own targets out of the default build --
  # otherwise `make build` would compile them even though nothing in ALL links
  # them. They build only as dependencies of bitboard_test (under `make test`).
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(googletest)
endif()

# EXCLUDE_FROM_ALL: the test binary is deliberately not part of the default
# build. Building the deliverable (`make build`) therefore never compiles
# GoogleTest or any test code; `make test` builds this target explicitly and
# runs it via ctest.
add_executable(bitboard_test EXCLUDE_FROM_ALL bitboard_test.cpp)

# The test TU imports Tilted.Bitboard (which internally `import std;`), so the
# std module must be reachable when linking; and it links the module-providing
# library plus GoogleTest.
set_target_properties(bitboard_test PROPERTIES CXX_MODULE_STD ON)
target_link_libraries(bitboard_test PRIVATE tilted GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(bitboard_test)
