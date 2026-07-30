#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <vector>
#include <string>
#include <functional>
#include "CudaBuffer.h"
#include "NBodySystem.h"
#include "kernels/accelerations.cuh" 

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

struct GpuBenchmarkResult {
    int    n_bodies;
    int    variant;       // 0 = básica, 1 = shared memory
    int    blockDim;
    std::string measureType; // "kernel-only" o "end-to-end"
    double avgTime;
    double stdDevTime;
    double speedup;       // CPU_serial_time / GPU_time
    double cpuStdDev;     // desviación estándar del tiempo CPU serial
    double speedupError;  // error del speedup propagado según fórmula (4)
};


class Benchmark {
private:
    int numRepetitions;
    std::vector<BenchmarkResult>      results;
    std::vector<ChunkResult>          chunkResults;
    std::vector<PrivateSharedResult>  privateSharedResults;

    //Resultados Lab 2
    std::vector<GpuBenchmarkResult>   gpuResults;

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



    //Nuevos métodos GPU

    // Pruebas CPU vs GPU con tolerancia en coma flotante
    void compareCpuGpu(int n_bodies);

    // Mide el tiempo del kernel de aceleración excluyendo transferencias 
    // (sync incluida en la medición host)
    void benchmarkKernelOnly(int n_bodies, int variant, int blockDim);

    // Mide el tiempo del paso completo, incluyendo transferencias H2D y D2H
    void benchmarkEndToEnd(int n_bodies, int variant, int blockDim);

    // Para exportar 'benchmark_results.dat' y 'blockdim_study.dat'
    void saveGpuResultsToFile(const std::string& filename);
};

#endif
