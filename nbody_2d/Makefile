# =============================================================================
# Makefile — 2D N-Body Gravitational Simulator
# Laboratorio 1: Programación paralela con OpenMP
# Universidad de Santiago de Chile
# =============================================================================

# -----------------------------------------------------------------------------
# Compiler detection — works on macOS (Homebrew GCC) and Linux (system GCC)
# On macOS: Apple Clang does NOT support -fopenmp; use Homebrew GCC instead.
# On Linux (CI / Docker): system g++ supports -fopenmp natively.
# -----------------------------------------------------------------------------
UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
    # Main binary: Homebrew GCC (supports -fopenmp natively).
    # Override with:  make CXX=g++-15  if the detected version differs.
    CXX_HOMEBREW := $(shell ls /opt/homebrew/bin/g++-* 2>/dev/null | sort -V | tail -1)
    ifeq ($(CXX_HOMEBREW),)
        $(error "Homebrew GCC not found. Install with: brew install gcc")
    endif
    CXX = $(CXX_HOMEBREW)
    # Test binary: Apple Clang — must match the ABI used by Homebrew googletest
    # (brew install googletest compiles with libc++; GCC uses libstdc++, so they
    #  are ABI-incompatible.  clang++ links against the same libc++ as gtest.)
    #
    # CLTools 15+ / macOS 15 caveat:
    #   The libc++ headers (vector, string, …) are NO LONGER installed under
    #   CommandLineTools/usr/include/c++/v1/ — that directory now contains only
    #   ~11 private stub files (__bit, __compare, cxxabi.h …).
    #   The real public headers were moved into the active SDK, e.g.:
    #     MacOSX15.5.sdk/usr/include/c++/v1/vector
    #   clang++ does NOT add that SDK sub-path to its default search list, so
    #   '#include <vector>' fails with "file not found" unless we add it
    #   explicitly.  We query the active SDK at make-time via xcrun and prepend
    #   the SDK's c++/v1 directory to the include path.
    MACOS_SDK     := $(shell xcrun --show-sdk-path 2>/dev/null)
    TEST_CXX      = clang++
    TEST_CXXFLAGS = -Wall -Wextra -O2 -std=c++17 \
                    -I$(MACOS_SDK)/usr/include/c++/v1
    GTEST_INC     = -I/opt/homebrew/include
    GTEST_LIBS    = -L/opt/homebrew/lib -lgtest -lgtest_main
else
    CXX           = g++
    TEST_CXX      = g++
    TEST_CXXFLAGS = -Wall -Wextra -O2 -std=c++17
    # GoogleTest installed via: apt-get install libgtest-dev + cmake build
    GTEST_INC     =
    GTEST_LIBS    = -lgtest -lgtest_main
endif

CXXFLAGS = -Wall -Wextra -O3 -fopenmp -std=c++17
LDFLAGS  = -fopenmp

# -----------------------------------------------------------------------------
# Main binary
# -----------------------------------------------------------------------------
TARGET = nbody_2d

SOURCES = main.cpp          \
          Particle.cpp      \
          NBodySystem.cpp   \
          NBodySimulator.cpp\
          Integrator.cpp    \
          MetricsCalculator.cpp \
          Benchmark.cpp     \
          Visualizer.cpp

HEADERS = Particle.h      \
          NBodySystem.h   \
          NBodySimulator.h\
          Integrator.h    \
          MetricsCalculator.h \
          Benchmark.h     \
          Visualizer.h

OBJECTS = $(SOURCES:.cpp=.o)

# -----------------------------------------------------------------------------
# Test binary
# Links only the physics sources needed by the unit tests (no main.cpp,
# no Benchmark/Visualizer stubs).  Integrator.cpp is included so that
# IntegratorTest can call Integrator::eulerStep().
# GoogleTest provides its own main via -lgtest_main.
# -----------------------------------------------------------------------------
TEST_TARGET  = run_tests
TEST_SOURCES = tests/test_physics.cpp \
               Particle.cpp           \
               NBodySystem.cpp        \
               Integrator.cpp
TEST_OBJECTS = $(TEST_SOURCES:.cpp=.o)

# -----------------------------------------------------------------------------
# Phony targets
# -----------------------------------------------------------------------------
.PHONY: all clean test benchmark analysis help

# -----------------------------------------------------------------------------
# Default target
# -----------------------------------------------------------------------------
all: $(TARGET)

# -----------------------------------------------------------------------------
# Link main binary
# -----------------------------------------------------------------------------
$(TARGET): $(OBJECTS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

# -----------------------------------------------------------------------------
# Compile individual object files
# Pattern rule covers both top-level and tests/ subdirectory.
# -----------------------------------------------------------------------------
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# -----------------------------------------------------------------------------
# Build and run unit + integration tests
# The test binary is compiled entirely with TEST_CXX (clang++ on macOS,
# g++ on Linux) so its ABI matches the Homebrew / system googletest library.
# We compile the physics sources again here (from .cpp, not reusing .o files
# built with GCC) to guarantee a consistent ABI throughout the test binary.
# No -fopenmp needed: Week-1 tests exercise only serial physics code.
# -----------------------------------------------------------------------------
$(TEST_TARGET): tests/test_physics.cpp Particle.cpp NBodySystem.cpp Integrator.cpp $(HEADERS)
	$(TEST_CXX) $(TEST_CXXFLAGS) $(GTEST_INC) \
	    -o $@ \
	    tests/test_physics.cpp Particle.cpp NBodySystem.cpp Integrator.cpp \
	    $(GTEST_LIBS)

test: $(TEST_TARGET)
	@echo ""
	@echo "============================================"
	@echo " Running unit tests ($(TEST_CXX) + GoogleTest)..."
	@echo "============================================"
	./$(TEST_TARGET)
	@echo "============================================"
	@echo " Tests complete."
	@echo "============================================"

# -----------------------------------------------------------------------------
# Benchmarks (Week 4)
# -----------------------------------------------------------------------------
benchmark: $(TARGET)
	@echo ""
	@echo "============================================"
	@echo " Running benchmarks..."
	@echo "============================================"
	./$(TARGET) --benchmark

# -----------------------------------------------------------------------------
# Scaling / Amdahl analysis (Week 4)
# -----------------------------------------------------------------------------
analysis: $(TARGET)
	@echo ""
	@echo "============================================"
	@echo " Running scaling analysis..."
	@echo "============================================"
	./$(TARGET) --analysis

# -----------------------------------------------------------------------------
# Full simulation run (Week 3)
# -----------------------------------------------------------------------------
simulate: $(TARGET)
	./$(TARGET) --simulate

# -----------------------------------------------------------------------------
# Verify Week-1 physics (N=2, N=3)
# -----------------------------------------------------------------------------
verify: $(TARGET)
	./$(TARGET)

# -----------------------------------------------------------------------------
# Clean build artefacts and generated data/image files
# -----------------------------------------------------------------------------
clean:
	rm -f $(TARGET) $(TEST_TARGET)
	rm -f $(OBJECTS) tests/test_physics.o
	rm -f *.dat *.png *.csv

# -----------------------------------------------------------------------------
# Help
# -----------------------------------------------------------------------------
help:
	@echo ""
	@echo "Available targets:"
	@echo "  all        — build the main binary (default)"
	@echo "  test       — compile and run unit tests"
	@echo "  verify     — run Week-1 physics verification (N=2, N=3)"
	@echo "  simulate   — run full simulation (Week 3)"
	@echo "  benchmark  — run OpenMP benchmarks (Week 4)"
	@echo "  analysis   — run speedup/Amdahl analysis (Week 4)"
	@echo "  clean      — remove compiled files and generated data"
	@echo "  help       — show this message"
	@echo ""
	@echo "Compiler flags: $(CXXFLAGS)"
	@echo ""
