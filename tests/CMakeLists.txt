# Always build GoogleTest from source under build/_deps
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)

# EXCLUDE_FROM_ALL keeps the test binaries out of the default build: `make build`
# never compiles GoogleTest or test code; `make test` builds them explicitly.
# Each imports Tilted modules (which use `import std;`), so the std module must be
# reachable (CXX_MODULE_STD); each links the module library plus GoogleTest.
add_executable(bitboard_test EXCLUDE_FROM_ALL bitboard_test.cpp)
set_target_properties(bitboard_test PROPERTIES CXX_MODULE_STD ON)
target_link_libraries(bitboard_test PRIVATE tilted GTest::gtest_main)

add_executable(attacks_test EXCLUDE_FROM_ALL attacks_test.cpp)
set_target_properties(attacks_test PROPERTIES CXX_MODULE_STD ON)
target_link_libraries(attacks_test PRIVATE tilted GTest::gtest_main)

# Aggregate so `make test` can build every suite with one target.
add_custom_target(unit_tests DEPENDS bitboard_test attacks_test)

include(GoogleTest)
gtest_discover_tests(bitboard_test)
gtest_discover_tests(attacks_test)
