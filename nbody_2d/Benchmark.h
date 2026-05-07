#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include <functional>

// Resultado de un punto del scaling analysis
struct BenchmarkResult {
    int    numThreads;
    double avgTime;
    double stdDevTime;
    double speedup;
    double speedupError;
    double efficiency;
    double efficiencyError;
    double serialFraction;   // f_s = (1/Sp - 1/p) / (1 - 1/p)   [Amdahl]
    double parallelFraction; // f_p = 1 - f_s
};

// Resultado de una combinación (schedule × chunk size)
struct ChunkResult {
    int    scheduleType;   // 1=static, 2=dynamic, 3=guided
    int    chunkSize;
    double avgTime;
    double stdDevTime;
};

// Resultado del benchmark private vs shared
struct PrivateSharedResult {
    std::string label;   // "private" o "shared"
    int    numThreads;
    double avgTime;
    double stdDevTime;
    double speedupVsSerial; // relativo al tiempo serial de referencia
};

class Benchmark {
private:
    int numRepetitions;
    std::vector<BenchmarkResult>      results;
    std::vector<ChunkResult>          chunkResults;
    std::vector<PrivateSharedResult>  privateSharedResults;

    // Método interno para ejecutar un experimento P veces
    void runExperiment(int numThreads, const std::function<void(bool)>& func,
                       double& avgTime, double& stdDevTime,
                       bool fork_join_outside = false);

public:
    explicit Benchmark(int repetitions = 10);

    // Versión interna sin argumento bool (para chunk analysis y private/shared)
    void runExperimentSimple(const std::function<void()>& func,
                             double& avgTime, double& stdDevTime);

    // Ejecuta análisis de escalabilidad de 1 hasta maxThreads
    void runScalingAnalysis(int maxThreads,
                            const std::function<void(bool)>& func,
                            bool fork_join_outside = false);

    // Itera sobre schedules × chunkSizes con hilos fijos y llena chunkResults.
    // func recibe (scheduleType, chunkSize).
    void runChunkAnalysis(int numThreads,
                          std::vector<int> chunkSizes,
                          std::vector<int> schedules,
                          const std::function<void(int, int)>& func);

    // Benchmark private vs shared:
    //   funcPrivate  → versión con cláusula private
    //   funcShared   → versión con cláusula shared
    //   serialRefTime → T1 de referencia para speedup
    void runPrivateVsShared(int numThreads,
                            double serialRefTime,
                            const std::function<void()>& funcPrivate,
                            const std::function<void()>& funcShared);

    // Exportar resultados de scaling analysis (incluye fracciones serial y paralela)
    void saveResultsToFile(const std::string& filename);

    // Exportar resultados de chunk analysis
    void saveChunkResultsToFile(const std::string& filename);

    // Exportar resultados de private vs shared
    void savePrivateSharedToFile(const std::string& filename);
};

#endif
