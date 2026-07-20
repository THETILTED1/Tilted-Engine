CLANG_FORMAT ?= clang-format
FORMAT_STYLE := {BasedOnStyle: LLVM, IndentWidth: 4}
FORMAT_SRC := $(shell find src -type f \( -name '*.h' -o -name '*.inl' -o -name '*.cpp' -o -name '*.cppm' \))

# Modules use clang-only flags; default to clang++ but let `make CXX=...` win
# (?= can't override make's built-in g++ default).
ifeq ($(origin CXX),default)
CXX := clang++
endif
CXXFLAGS ?= -std=c++23 -O2 -Wall -Wextra

# Quiet the expected noise from building libstdc++ headers as (experimental) units.
MODULE_FLAGS := -Wno-experimental-header-units -Wno-pragma-system-header-outside-header \
                -Wno-deprecated-builtins -Wno-keyword-compat

BUILD := build
IMPORTS := -fprebuilt-module-path=$(BUILD) $(MODULE_FLAGS)

# Standard headers re-exported through Tilted.Std -- extend as the engine grows.
STD_HEADERS := array bit cstddef cstdint iostream type_traits unordered_map
STD_PCM := $(addprefix $(BUILD)/, $(addsuffix .pcm, $(STD_HEADERS)))

TARGET := $(BUILD)/tilted
ENGINE_SRC := $(wildcard src/*.cpp)
ENGINE_OBJ := $(patsubst src/%.cpp, $(BUILD)/%.o, $(ENGINE_SRC))

.PHONY: all std format clean
all: std $(ENGINE_OBJ)

std: $(BUILD)/Tilted.Std.pcm $(BUILD)/Tilted.Std.o

format:
	$(CLANG_FORMAT) -i -style="$(FORMAT_STYLE)" $(FORMAT_SRC)

clean:
	rm -rf $(BUILD)

$(BUILD):
	mkdir -p $(BUILD)

# <hdr>  ->  build/<hdr>.pcm     (header-unit BMI, built from the system header)
$(BUILD)/%.pcm: | $(BUILD)
	$(CXX) $(CXXFLAGS) $(MODULE_FLAGS) -fmodule-header=system -xc++-system-header $* -o $@

# Tilted.Std interface  ->  BMI  ->  object
$(BUILD)/Tilted.Std.pcm: src/Tilted.Std.cppm $(STD_PCM) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(MODULE_FLAGS) $(addprefix -fmodule-file=, $(STD_PCM)) --precompile $< -o $@

$(BUILD)/Tilted.Std.o: $(BUILD)/Tilted.Std.pcm
	$(CXX) $(CXXFLAGS) $(MODULE_FLAGS) -c $< -o $@

# engine TU  ->  object     (sees the modules via -fprebuilt-module-path)
$(BUILD)/%.o: src/%.cpp $(BUILD)/Tilted.Std.pcm | $(BUILD)
	$(CXX) $(CXXFLAGS) $(IMPORTS) -c $< -o $@

# link the engine (needs a main; wired up for when the sources exist)
$(TARGET): $(ENGINE_OBJ) $(BUILD)/Tilted.Std.o
	$(CXX) $(CXXFLAGS) $^ -o $@
