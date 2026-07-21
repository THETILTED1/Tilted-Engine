cmake_minimum_required(VERSION 3.30)

# Experimental `import std;` gate; the UUID is version-specific (this is CMake 4.4's).
# If you bump CMake, configure prints the UUID it now wants -- paste it here.
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")

project(Tilted CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd (see .clangd)

# Stdlib choice lives in the presets, not here: it must be set before project()
# or CMake's `import std;` detection picks the wrong std module (clang->libc++, gcc->libstdc++).

# Module interface units; CMake derives the build order itself. Bitboard.inl is a
# plain #include of Tilted.Bitboard.cppm, so it is intentionally not listed.
add_library(tilted STATIC)
target_sources(tilted
  PRIVATE
    FILE_SET CXX_MODULES FILES
      src/Tilted.Consts.cppm
      src/Tilted.Util.cppm
      src/Tilted.Bitboard.cppm
      src/Tilted.Attacks.cppm
      src/Tilted.Position.cppm
)

# Build the stdlib as a module so `import std;` resolves (libc++'s std.cppm on clang).
set_target_properties(tilted PROPERTIES CXX_MODULE_STD ON)
