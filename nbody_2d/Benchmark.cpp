#include "Benchmark.h"
#include <omp.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>

Benchmark::Benchmark(int repetitions) : numRepetitions(repetitions) {}

void Benchmark::runExperiment(int numThreads, const std::function<void(bool)>& func, double& avgTime, double& stdDevTime, bool fork_join_outside) {
    omp_set_num_threads(numThreads);
    
    // Usaremos lotes para diluir el ruido del OS y mejorar la estabilidad estadística
    int numBatches = 5;
    int repsPerBatch = numRepetitions; // repeticiones por lote
    
    std::vector<double> batchTimes(numBatches);
    double sumTimes = 0.0;
    
    if (fork_join_outside) {
        #pragma omp parallel
        {
            // Warm-up: calentar caches y levantar frecuencia del CPU
            for (int w = 0; w < 5; ++w) {
                func(true); // true = inside_parallel
            }
            #pragma omp barrier
            
            for (int b = 0; b < numBatches; ++b) {
                double start_time = 0.0;
                #pragma omp master
                start_time = omp_get_wtime();
                
                for (int i = 0; i < repsPerBatch; ++i) {
                    func(true);
                }
                
                #pragma omp barrier
                #pragma omp master
                {
                    double end_time = omp_get_wtime();
                    // Tiempo promedio de 1 iteración en este lote
                    batchTimes[b] = (end_time - start_time) / repsPerBatch;
                    sumTimes += batchTimes[b];
                }
                #pragma omp barrier // esperar a que termine el master
            }
        }
    } else {
        // Warm-up
        for (int w = 0; w < 5; ++w) {
            func(false);
        }
        
        for (int b = 0; b < numBatches; ++b) {
            double start_time = omp_get_wtime();
            
            for (int i = 0; i < repsPerBatch; ++i) {
                func(false);
            }
            
            double end_time = omp_get_wtime();
            batchTimes[b] = (end_time - start_time) / repsPerBatch;
            sumTimes += batchTimes[b];
        }
    }
    
    // Promedio de tiempo (T) sobre los lotes
    avgTime = sumTimes / numBatches;
    
    // Desviacion estandar (sigma_T) de los promedios de los lotes
    double sumSqDiff = 0.0;
    for (int b = 0; b < numBatches; ++b) {
        double diff = batchTimes[b] - avgTime;
        sumSqDiff += diff * diff;
    }
    
    if (numBatches > 1) {
        stdDevTime = std::sqrt(sumSqDiff / (numBatches - 1));
    } else {
        stdDevTime = 0.0;
    }
}

void Benchmark::runScalingAnalysis(int maxThreads, const std::function<void(bool)>& func, bool fork_join_outside) {
    results.clear();
    
    double t1_avg = 0.0;
    double t1_stddev = 0.0;
    
    for (int p = 1; p <= maxThreads; ++p) {
        double avgTime, stdDevTime;
        runExperiment(p, func, avgTime, stdDevTime, fork_join_outside);
        
        BenchmarkResult res;
        res.numThreads = p;
        res.avgTime = avgTime;
        res.stdDevTime = stdDevTime;
        
        if (p == 1) {
            // T1
            t1_avg = avgTime;
            t1_stddev = stdDevTime;
            
            res.speedup = 1.0;
            res.speedupError = 0.0; 
            res.efficiency = 1.0;
            res.efficiencyError = 0.0;
            res.serialFraction = 1.0; // Todo serial
        } else {
            // Calculo de Speedup: Sp = T1 / Tp
            res.speedup = t1_avg / avgTime;
            
            // Error de Speedup: sigma_Sp = Sp * sqrt( (sigma_T1 / T1)^2 + (sigma_Tp / Tp)^2 )
            double rel_err_t1 = t1_stddev / t1_avg;
            double rel_err_tp = stdDevTime / avgTime;
            res.speedupError = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 + rel_err_tp * rel_err_tp);
            
            // Calculo de Eficiencia: Ep = Sp / p
            res.efficiency = res.speedup / p;
            
            // Error de Eficiencia: sigma_Ep = sigma_Sp / p
            res.efficiencyError = res.speedupError / p;
            
            // Fraccion serial (Amdahl): Sp = 1 / (f + (1-f)/p)  =>  f = (1/Sp - 1/p) / (1 - 1/p)
            res.serialFraction = (1.0 / res.speedup - 1.0 / p) / (1.0 - 1.0 / p);
        }
        
        results.push_back(res);
        std::cout << "Threads: " << p 
                  << " | Time: " << avgTime << "s (+-" << stdDevTime << ")"
                  << " | Sp: " << res.speedup 
                  << " | Ep: " << res.efficiency << "\n";
    }
}

void Benchmark::saveResultsToFile(const std::string& filename) {
    std::ofstream out(filename);
    if (out.is_open()) {
        // Cabecera
        out << "Threads AvgTime StdDevTime Speedup SpeedupError Efficiency EfficiencyError SerialFraction\n";
        
        // Datos
        for (const auto& res : results) {
            out << std::fixed << std::setprecision(6)
                << res.numThreads << " "
                << res.avgTime << " "
                << res.stdDevTime << " "
                << res.speedup << " "
                << res.speedupError << " "
                << res.efficiency << " "
                << res.efficiencyError << " "
                << res.serialFraction << "\n";
        }
        out.close();
        std::cout << "Resultados guardados exitosamente en: " << filename << "\n";
    } else {
        std::cerr << "Error: No se pudo abrir el archivo " << filename << " para guardar resultados de benchmark.\n";
    }
}
