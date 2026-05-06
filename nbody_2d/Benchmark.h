#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include <functional>

struct BenchmarkResult {
    int numThreads;
    double avgTime;
    double stdDevTime;
    double speedup;
    double speedupError;
    double efficiency;
    double efficiencyError;
    double serialFraction;
};

// Resultado de una combinación (schedule × chunk size)
struct ChunkResult {
    int    scheduleType;   // 1=static, 2=dynamic, 3=guided
    int    chunkSize;
    double avgTime;
    double stdDevTime;
};

class Benchmark {
private:
    int numRepetitions;
    std::vector<BenchmarkResult> results;
    std::vector<ChunkResult>     chunkResults;

    // Método interno para ejecutar un experimento P veces
    void runExperiment(int numThreads, const std::function<void(bool)>& func, double& avgTime, double& stdDevTime, bool fork_join_outside = false);

    // Versión interna que acepta una función sin argumento bool (para chunk analysis)
    void runExperimentSimple(const std::function<void()>& func, double& avgTime, double& stdDevTime);

public:
    Benchmark(int repetitions = 10);

    // Ejecuta analisis de escalabilidad de 1 hasta maxThreads
    void runScalingAnalysis(int maxThreads, const std::function<void(bool)>& func, bool fork_join_outside = false);

    // Itera sobre schedules × chunkSizes con hilos fijos y llena chunkResults.
    // func recibe (scheduleType, chunkSize) — debe llamar a computeAccelerations internamente.
    void runChunkAnalysis(int numThreads,
                          std::vector<int> chunkSizes,
                          std::vector<int> schedules,
                          const std::function<void(int, int)>& func);

    // Exportar a .dat (ej. benchmark_results.dat, scaling_analysis.dat)
    void saveResultsToFile(const std::string& filename);

    // Exportar resultados de chunk analysis a fichero
    void saveChunkResultsToFile(const std::string& filename);
};

#endif
