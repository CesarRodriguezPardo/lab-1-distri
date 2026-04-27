# ─────────────────────────────────────────────────────────────
#  Makefile — N-Body 2D Simulator
#  Rol 5: Calidad, CI y Visualización
# ─────────────────────────────────────────────────────────────

# OMP flags
OMPFLAGS = -fopenmp

# Compilador y flags base
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(OMPFLAGS)

# En macOS (Darwin) se necesita apuntar al SDK de Xcode y usar libc++
# para que el compilador encuentre los headers de la stdlib (iostream, vector…)
# En Linux (Ubuntu/CI) esto no es necesario.
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CXXFLAGS += -stdlib=libc++ -isysroot $(shell xcrun --show-sdk-path)
endif

# ─────────────────────────────────────────────────────────────
#  Fuentes
# ─────────────────────────────────────────────────────────────

# Fuentes del simulador (sin main.cpp → reutilizables en tests)
SRCS = Particle.cpp \
       NBodySystem.cpp \
       NBodySimulator.cpp

# Fuentes de tests:
#  - catch_amalgamated.cpp  → implementación de Catch2 v3 (incluye main())
#  - test_Particle.cpp      → pruebas unitarias de Particle
#  - test_NBodySystem.cpp   → pruebas de integración NBodySystem/Simulator
TEST_SRCS = tests/catch_amalgamated.cpp \
            tests/test_Particle.cpp \
            tests/test_NBodySystem.cpp

# ─────────────────────────────────────────────────────────────
#  Targets
# ─────────────────────────────────────────────────────────────
.PHONY: all test clean

# Target por defecto: compila el binario principal
all: nbody

nbody: main.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# make test:
#   1. Compila SRCS + TEST_SRCS → test_runner
#      (catch_amalgamated.cpp provee el main(), NO se usa main.cpp)
#   2. Ejecuta ./test_runner → Catch2 reporta resultados por stdout
#   3. Exit code != 0 si algún test falla → CI detecta el fallo
test: $(SRCS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o test_runner
	./test_runner

# make clean: elimina binarios y datos generados
clean:
	rm -f nbody test_runner *.dat
