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
    void runExperiment(int numThreads, const std::function<void()>& func, double& avgTime, double& stdDevTime);

public:
    Benchmark(int repetitions = 10);
    
    // Ejecuta analisis de escalabilidad de 1 hasta maxThreads
    void runScalingAnalysis(int maxThreads, const std::function<void()>& func);
    
    // Exportar a .dat (ej. benchmark_results.dat, scaling_analysis.dat)
    void saveResultsToFile(const std::string& filename);
};

#endif
