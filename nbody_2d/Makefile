
# OMP flags
OMPFLAGS = -fopenmp

# Compilador y flags base
CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra $(OMPFLAGS)

# ─────────────────────────────────────────────────────────────
#  Fuentes
# ─────────────────────────────────────────────────────────────

# Fuentes del simulador (sin main.cpp → reutilizables en tests)
SRCS = Particle.cpp \
       NBodySystem.cpp \
       NBodySimulator.cpp \
       MetricsCalculator.cpp \
       Benchmark.cpp

# Fuentes de tests:
#  - catch_amalgamated.cpp  → implementación de Catch2 v3 (incluye main())
#  - test_Particle.cpp      → pruebas unitarias de Particle
#  - test_NBodySystem.cpp   → pruebas de integración NBodySystem/Simulator
TEST_SRCS = tests/catch_amalgamated.cpp \
            tests/test_Particle.cpp \
            tests/test_NBodySystem.cpp \
            tests/test_MetricsCalculator.cpp

# Directorio de scripts Python
SCRIPTS_DIR = scripts

# ─────────────────────────────────────────────────────────────
#  Targets
# ─────────────────────────────────────────────────────────────
.PHONY: all test analysis plot clean benchmark

# Target por defecto: compila el binario principal
all: nbody

nbody: main.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

benchmark: benchmark_main.cpp $(SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# make test:
#   1. Compila SRCS + TEST_SRCS → test_runner
#      (catch_amalgamated.cpp provee el main(), NO se usa main.cpp)
#   2. Ejecuta ./test_runner → Catch2 reporta resultados por stdout
#   3. Exit code != 0 si algún test falla → CI detecta el fallo
test: $(SRCS) $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o test_runner
	./test_runner

# make analysis:
#   1. Compila el binario principal si no está actualizado (dependencia: nbody)
#   2. Ejecuta la simulación → genera dos archivos de datos:
#        trajectories.dat      — posición y velocidad de cada cuerpo por paso
#        energy_timeseries.dat — K(t), U(t), E(t) de cada paso
#   Nota: si los .dat ya existen de una corrida anterior, make los sobreescribirá
#   porque 'nbody' es el target de prerequisito (no los .dat en sí).
analysis: nbody
	@echo "==> Ejecutando simulación para generar archivos de datos..."
	./nbody
	@echo "==> Archivos generados:"
	@ls energy_*.dat trajectories_*.dat 2>/dev/null | sed 's/^/     - /' || echo "     (ningún archivo .dat encontrado)"

# make plot:
#   1. Llama a make analysis (asegura que los .dat existen y son frescos)
#   2. Ejecuta el script de energía → genera energy_plot.png
#      Lee energy_timeseries.dat; muestra K(t), U(t), E(t) y anotación de deriva.
#   3. Ejecuta el script de trayectorias → genera:
#        trajectories_plot.png   — trayectorias en el plano (x,y) con gradiente temporal
#        position_vs_time.png    — x(t) e y(t) del cuerpo 0
#   Requisito del entorno: gnuplot instalado.
#     macOS  → brew install gnuplot
#     Debian → apt-get install gnuplot   (ya incluido en el Dockerfile)
plot: analysis
	@echo "==> Generando gráfico de energía..."
	gnuplot $(SCRIPTS_DIR)/plot_energy.gnu
	@echo "==> Generando gráficos de trayectorias..."
	gnuplot $(SCRIPTS_DIR)/plot_trajectories.gnu
	@echo "==> Generando gráfico de chunk analysis..."
	gnuplot $(SCRIPTS_DIR)/plot_chunk.gnu
	@echo "==> Gráficos listos:"
	@echo "     - energy_plot.png"
	@echo "     - trajectories_plot.png"
	@echo "     - position_vs_time.png"
	@echo "     - chunk_plot.png"

# make clean: elimina binarios, datos generados y figuras
clean:
	rm -f nbody test_runner *.dat *.png

