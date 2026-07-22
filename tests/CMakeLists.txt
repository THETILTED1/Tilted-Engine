# Always build GoogleTest from source under build/_deps
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)

# EXCLUDE_FROM_ALL keeps the test binary out of the default build: `make build`
# never compiles GoogleTest or test code; `make test` builds it explicitly.
add_executable(bitboard_test EXCLUDE_FROM_ALL bitboard_test.cpp)

# Imports Tilted.Bitboard (which uses `import std;`), so the std module must be
# reachable (CXX_MODULE_STD); links the module library plus GoogleTest.
set_target_properties(bitboard_test PROPERTIES CXX_MODULE_STD ON)
target_link_libraries(bitboard_test PRIVATE tilted GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(bitboard_test)
