# Convenience build file. Plain g++ commands also work (see README.md).
#   make          build ./library_manager
#   make test     build and run the unit tests and the end-to-end CLI tests
#   make clean    remove build outputs

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow

LIB_SRC  := $(filter-out src/main.cpp,$(wildcard src/*.cpp))

library_manager: $(wildcard src/*.cpp) $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) src/*.cpp -o library_manager

unit_tests: $(LIB_SRC) tests/unit_tests.cpp $(wildcard src/*.h)
	$(CXX) $(CXXFLAGS) -Isrc $(LIB_SRC) tests/unit_tests.cpp -o unit_tests

test: library_manager unit_tests
	./unit_tests
	bash tests/run_cli_tests.sh ./library_manager

clean:
	rm -f library_manager unit_tests library_manager.exe unit_tests.exe

.PHONY: test clean
