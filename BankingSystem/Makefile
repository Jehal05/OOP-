# Nova Bank - Banking & Management System
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
# On Windows (MinGW) link the runtime statically so a different libstdc++-6.dll on PATH cannot break startup
ifeq ($(OS),Windows_NT)
LDFLAGS  += -static
endif
INC      := -Iinclude
BUILD    := build
SRC      := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJ      := $(patsubst src/%.cpp,$(BUILD)/%.o,$(SRC))
TESTS    := $(patsubst tests/%.cpp,$(BUILD)/%,$(wildcard tests/test_*.cpp))

.PHONY: all run test valgrind diagrams check clean
all: bank

bank: $(OBJ) $(BUILD)/main.o
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD)/%.o: src/%.cpp $(wildcard include/*.h) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/test_%: tests/test_%.cpp $(OBJ) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -Itests $< $(OBJ) -o $@ $(LDFLAGS)

run: bank
	./bank

test: $(TESTS)
	@for t in $(TESTS); do echo "== $$t"; ./$$t || exit 1; done; echo "ALL TEST SUITES PASSED"

valgrind: bank        ## every scripted scenario + integration tests under Valgrind
	./tools/valgrind_all.sh

diagrams:             ## regenerate docs/img/*.png from docs/diagrams/*.mmd (needs mermaid-cli: mmdc)
	cd docs/diagrams && for f in *.mmd; do mmdc -q -i $$f -o ../img/$${f%.mmd}.png -p puppeteer.json -b white -s 2; done

check: test valgrind   ## run everything

clean:
	rm -rf $(BUILD) bank
