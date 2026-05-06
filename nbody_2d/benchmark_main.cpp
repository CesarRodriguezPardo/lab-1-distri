#include <iostream>
#include <vector>
#include <sstream>
#include "NBodySystem.h"
#include "MetricsCalculator.h"
#include "Benchmark.h"
#include "NBodySimulator.h"

int main() {
    std::cout << "Inicializando sistema para Benchmark (500 particulas)...\n";
    NBodySystem system(1.0, 0.05); // G=1.0 para que se noten los valores
    system.randomSystem(500, 42);

    std::cout << "Probando MetricsCalculator...\n";
    MetricsCalculator metrics(&system);
    system.computeAccelerations();
    metrics.calculateAllMetrics();
    metrics.saveMetricsToFile("energy_timeseries.dat", 0.0);
    
    std::cout << "Metricas calculadas:\n";
    std::cout << " - K: " << metrics.getKineticEnergy() << "\n";
    std::cout << " - U: " << metrics.getPotentialEnergy() << "\n";
    std::cout << " - E: " << metrics.getTotalEnergy() << "\n";
    std::cout << " - Centro de Masas X: " << metrics.getCmX() << " Y: " << metrics.getCmY() << "\n";
    std::cout << " - Momentum Magnitude: " << metrics.getMomentumMagnitude() << "\n";
    std::cout << " - Min Distance: " << metrics.getMinDistance() << "\n";
    std::cout << " - RMS Radius: " << metrics.getRmsRadius() << "\n";

    std::cout << "\nEjecutando Scaling Analysis (NBodySimulator)...\n";
    NBodySimulator simulator(&system, 0.01);
    std::stringstream dummyStream;

    std::cout << " -> Modo PARALELO (omp for, nowait, static)\n";
    Benchmark benchParallel(20);
    benchParallel.runScalingAnalysis(4, [&](bool /*inside_parallel*/) {
        // sim_type=1 (parallel), syncType=2 (nowait), schedule=1 (static)
        simulator.processBodies(dummyStream, 1, 2, 1, 0); 
    }, false); // El simulador maneja sus propias regiones paralelas
    benchParallel.saveResultsToFile("scaling_analysis.dat");

    std::cout << "\n -> Modo TAREAS (omp task)\n";
    Benchmark benchTasks(20);
    benchTasks.runScalingAnalysis(4, [&](bool /*inside_parallel*/) {
        // sim_type=2 (tasks)
        simulator.processBodies(dummyStream, 2, 2, 1, 0);
    }, false);
    benchTasks.saveResultsToFile("scaling_tasks.dat");

    // ─────────────────────────────────────────────────────────────
    //  Módulo 3 — Benchmark: Tiempo vs. Chunk × Schedule
    // ─────────────────────────────────────────────────────────────
    std::cout << "\nEjecutando Chunk-Schedule Analysis (4 hilos fijos, 20 repeticiones)...\n";

    int fixedThreads = 4;
    std::vector<int> chunkSizes = {1, 4, 16, 64, 256};
    std::vector<int> schedules  = {1, 2, 3};  // 1=static, 2=dynamic, 3=guided

    Benchmark benchChunk(20);
    benchChunk.runChunkAnalysis(fixedThreads, chunkSizes, schedules,
        [&](int sched, int chunk) {
            // Benchmarkamos el paso completo usando el simulador
            simulator.processBodies(dummyStream, 1, 2, sched, chunk);
        });
    benchChunk.saveChunkResultsToFile("benchmark_results.dat");

    return 0;
}
