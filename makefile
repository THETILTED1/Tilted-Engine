# Convenience wrapper; CMake + Ninja is the real build system (see CMakeLists.txt
# / CMakePresets.json). Just task shortcuts.

CMAKE  ?= cmake
CTEST  ?= ctest
PRESET ?= clang
BUILD  := build

CLANG_FORMAT ?= clang-format
FORMAT_STYLE := {BasedOnStyle: LLVM, IndentWidth: 4}
FORMAT_SRC := $(shell find src -type f \( -name '*.h' -o -name '*.inl' -o -name '*.cpp' -o -name '*.cppm' \))

.PHONY: all build test configure reconfigure clean format

# Default: configure once (if needed), then build.
all: build

build: $(BUILD)/CMakeCache.txt
	$(CMAKE) --build --preset $(PRESET)

# Build just the test binary (kept out of the default build) and run it. The
# deliverable build (`make build`) stays free of test code and GoogleTest.
test: $(BUILD)/CMakeCache.txt
	$(CMAKE) --build --preset $(PRESET) --target unit_tests
	$(CTEST) --test-dir $(BUILD) --output-on-failure

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
