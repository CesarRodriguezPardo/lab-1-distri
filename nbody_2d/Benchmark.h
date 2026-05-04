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

class Benchmark {
private:
    int numRepetitions;
    std::vector<BenchmarkResult> results;
    
    // Método interno para ejecutar un experimento P veces
    void runExperiment(int numThreads, const std::function<void(bool)>& func, double& avgTime, double& stdDevTime, bool fork_join_outside = false);

public:
    Benchmark(int repetitions = 10);
    
    // Ejecuta analisis de escalabilidad de 1 hasta maxThreads
    void runScalingAnalysis(int maxThreads, const std::function<void(bool)>& func, bool fork_join_outside = false);
    
    // Exportar a .dat (ej. benchmark_results.dat, scaling_analysis.dat)
    void saveResultsToFile(const std::string& filename);
};

#endif
