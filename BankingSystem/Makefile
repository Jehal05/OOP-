# Nova Bank - Banking & Management System
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
# On Windows (MinGW) link the runtime statically so a different libstdc++-6.dll on PATH cannot break startup
ifeq ($(OS),Windows_NT)
LDFLAGS  += -static
WINLIBS  := -lws2_32 -lwsock32
endif
INC      := -Iinclude
BUILD    := build
SRC      := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJ      := $(patsubst src/%.cpp,$(BUILD)/%.o,$(SRC))
TESTS    := $(patsubst tests/%.cpp,$(BUILD)/%,$(wildcard tests/test_*.cpp))

.PHONY: all web run test valgrind widths screenshots diagrams check clean
all: bank

# Localhost web version (same Bank classes behind a JSON API + browser UI)
bankweb: web/server.cpp $(OBJ) web/third_party/httplib.h
	$(CXX) $(CXXFLAGS) $(INC) -isystem web/third_party $< $(OBJ) -o $@ -pthread $(LDFLAGS) $(WINLIBS)

web: bankweb
	./bankweb

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

widths: bank          ## fail if any screen line exceeds 80 columns
	python3 tools/check_width.py

screenshots: bank     ## regenerate docs/img/screens/*.png from the real binary
	python3 tools/make_screenshots.py

diagrams:             ## regenerate docs/img/*.png from docs/diagrams/*.mmd (needs mermaid-cli: mmdc)
	cd docs/diagrams && for f in *.mmd; do mmdc -q -i $$f -o ../img/$${f%.mmd}.png -p puppeteer.json -b white -s 2; done

check: test widths valgrind   ## run everything

clean:
	rm -rf $(BUILD) bank bankweb
