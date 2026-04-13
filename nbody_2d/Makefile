# Compilador y flags base
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

############################################################

# fuentes de codigo a compilar
SRCS = Particle.cpp \
       NBodySystem.cpp \
       NBodySimulator.cpp

# fuentes de codigo a testear
TEST_SRCS = tests/catch_amalgamated.cpp \
            tests/test_Particle.cpp \
            tests/test_NBodySystem.cpp

############################################################
# make all ejecuta el binario

all: nbody

nbody: main.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

############################################################
# make test ejecuta los tests

test: $(SRCS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o test_runner
	./test_runner

############################################################
# make clean limpia binarios y archivos creados

clean:
	rm -f nbody test_runner *.dat
