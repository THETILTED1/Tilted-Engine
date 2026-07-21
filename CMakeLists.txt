cmake_minimum_required(VERSION 4.4)

# --- `import std;` support -------------------------------------------------
# Still experimental in CMake; the gate value is specific to the CMake version
# (this one is CMake 4.4). Both settings must precede the CXX language being
# enabled (i.e. before project()).
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")

# LLVM-22's apt libc++ ships a libc++.modules.json in the multiarch lib dir
# whose relative source-paths resolve to a nonexistent /usr/lib/share/libc++.
# An identical copy sits in the LLVM tree where the relative paths *do* resolve
# (../share/libc++/v1 -> /usr/lib/llvm-22/share/libc++/v1). Point CMake at that
# one so it can find std.cppm / std.compat.cppm.
if(NOT DEFINED CMAKE_CXX_STDLIB_MODULES_JSON
   AND EXISTS "/usr/lib/llvm-22/lib/libc++.modules.json")
  set(CMAKE_CXX_STDLIB_MODULES_JSON "/usr/lib/llvm-22/lib/libc++.modules.json")
endif()

project(Tilted CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # for clangd

# `import std;` on this toolchain requires libc++ (libstdc++ ships no std
# module source). Keep compile and link stdlib in lockstep.
add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++)

# Engine module interface units. CMake scans these and derives the build order
# (Consts -> Bitboard -> Attacks -> Position; Util is a leaf) itself -- no
# hand-maintained dependency graph. Bitboard.inl is #included by
# Tilted.Bitboard.cppm and tracked as an ordinary include dependency, so it is
# deliberately NOT listed here.
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
set_target_properties(tilted PROPERTIES CXX_MODULE_STD ON)
