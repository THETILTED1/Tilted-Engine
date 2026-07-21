# Convenience wrapper around the CMake build. CMake + Ninja is the real build
# system (see CMakeLists.txt / CMakePresets.json); this Makefile is just task
# shortcuts and repo housekeeping.

CMAKE  ?= cmake
PRESET ?= default
BUILD  := build

CLANG_FORMAT ?= clang-format
FORMAT_STYLE := {BasedOnStyle: LLVM, IndentWidth: 4}
FORMAT_SRC := $(shell find src -type f \( -name '*.h' -o -name '*.inl' -o -name '*.cpp' -o -name '*.cppm' \))

.PHONY: all build configure reconfigure clean format

# Default: configure once (if needed), then build.
all: build

build: $(BUILD)/CMakeCache.txt
	$(CMAKE) --build --preset $(PRESET)

configure: $(BUILD)/CMakeCache.txt

# Configure marker -- created by the first `cmake --preset`.
$(BUILD)/CMakeCache.txt:
	$(CMAKE) --preset $(PRESET)

# Force a fresh configure (e.g. after editing CMakeLists.txt or adding modules).
reconfigure:
	$(CMAKE) --preset $(PRESET)

clean:
	rm -rf $(BUILD)

format:
	$(CLANG_FORMAT) -i -style="$(FORMAT_STYLE)" $(FORMAT_SRC)
