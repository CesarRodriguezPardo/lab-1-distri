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

# Directorio de scripts Gnuplot
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
	./benchmark

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
#        trajectories_<modo>.dat  — posición y velocidad de cada cuerpo por paso
#        energy_<modo>.dat        — K(t), U(t), E(t) de cada paso
analysis: nbody
	@echo "==> Ejecutando simulación para generar archivos de datos..."
	./nbody
	@echo "==> Archivos generados:"
	@ls energy_*.dat trajectories_*.dat 2>/dev/null | sed 's/^/     - /' || echo "     (ningún archivo .dat encontrado)"

# make plot:
#   1. Llama a make analysis (asegura que los .dat de simulación existen)
#   2. Si scaling_analysis.dat no existe, llama a make benchmark
#      (benchmark genera también benchmark_results.dat para plot_chunk)
#   3. Genera los 6 gráficos con gnuplot:
#        energy_plot.png        — K(t), U(t), E(t)
#        trajectories_plot.png  — trayectorias XY (primeros 8 cuerpos)
#        position_vs_time.png   — X(t) e Y(t) del cuerpo 0
#        speedup_plot.png       — Sp medido + ideal + predicción de Amdahl
#        efficiency_plot.png    — Ep medido + ideal + Amdahl + Tp vs hilos
#        chunk_plot.png         — Tiempo vs Chunk Size por schedule (static/dynamic/guided)
#   Requisito del entorno: gnuplot instalado.
#     macOS  → brew install gnuplot
#     Debian → apt-get install gnuplot   (ya incluido en el Dockerfile)
plot: analysis
	@[ -f scaling_analysis.dat ] || $(MAKE) benchmark
	@echo "==> Generando gráfico de energía..."
	gnuplot $(SCRIPTS_DIR)/plot_energy.gnu
	@echo "==> Generando gráficos de trayectorias..."
	gnuplot $(SCRIPTS_DIR)/plot_trajectories.gnu
	@echo "==> Generando gráfico de speedup + Amdahl..."
	gnuplot $(SCRIPTS_DIR)/plot_speedup.gnu
	@echo "==> Generando gráfico de eficiencia..."
	gnuplot $(SCRIPTS_DIR)/plot_efficiency.gnu
	@echo "==> Generando gráfico de chunk analysis..."
	gnuplot $(SCRIPTS_DIR)/plot_chunk.gnu
	@echo "==> Gráficos listos:"
	@echo "     - energy_plot.png"
	@echo "     - trajectories_plot.png"
	@echo "     - position_vs_time.png"
	@echo "     - speedup_plot.png"
	@echo "     - efficiency_plot.png"
	@echo "     - chunk_plot.png"

# make clean: elimina binarios, datos generados y figuras
clean:
	rm -f nbody test_runner *.dat *.png benchmark
