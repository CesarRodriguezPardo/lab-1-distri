#include "Benchmark.h"
#include <omp.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>

Benchmark::Benchmark(int repetitions) : numRepetitions(repetitions) {}

// ─────────────────────────────────────────────────────────────
//  runExperiment
//  Mide 'func' usando lotes de repeticiones para reducir el ruido del OS.
// ─────────────────────────────────────────────────────────────
void Benchmark::runExperiment(int numThreads, const std::function<void(bool)>& func,
                               double& avgTime, double& stdDevTime,
                               bool fork_join_outside) {
    omp_set_num_threads(numThreads);

    int numBatches   = 5;
    int repsPerBatch = numRepetitions;

    std::vector<double> batchTimes(numBatches);
    double sumTimes = 0.0;

    if (fork_join_outside) {
        #pragma omp parallel
        {
            // Warm-up
            for (int w = 0; w < 3; ++w) func(true);
            #pragma omp barrier

            for (int b = 0; b < numBatches; ++b) {
                double start_time = 0.0;
                #pragma omp master
                start_time = omp_get_wtime();

                for (int i = 0; i < repsPerBatch; ++i) func(true);

                #pragma omp barrier
                #pragma omp master
                {
                    double end_time = omp_get_wtime();
                    batchTimes[b] = (end_time - start_time) / repsPerBatch;
                    sumTimes += batchTimes[b];
                }
                #pragma omp barrier
            }
        }
    } else {
        // Warm-up
        for (int w = 0; w < 3; ++w) func(false);

        for (int b = 0; b < numBatches; ++b) {
            double start_time = omp_get_wtime();
            for (int i = 0; i < repsPerBatch; ++i) func(false);
            double end_time = omp_get_wtime();
            batchTimes[b] = (end_time - start_time) / repsPerBatch;
            sumTimes += batchTimes[b];
        }
    }

    avgTime = sumTimes / numBatches;

    double sumSqDiff = 0.0;
    for (int b = 0; b < numBatches; ++b) {
        double diff = batchTimes[b] - avgTime;
        sumSqDiff += diff * diff;
    }
    stdDevTime = (numBatches > 1) ? std::sqrt(sumSqDiff / (numBatches - 1)) : 0.0;
}

// ─────────────────────────────────────────────────────────────
//  runExperimentSimple
//  Versión sin argumento bool (para chunk analysis y private/shared).
// ─────────────────────────────────────────────────────────────
void Benchmark::runExperimentSimple(const std::function<void()>& func,
                                     double& avgTime, double& stdDevTime) {
    int numBatches   = 5;
    int repsPerBatch = numRepetitions;

    std::vector<double> batchTimes(numBatches);
    double sumTimes = 0.0;

    // Warm-up
    for (int w = 0; w < 3; ++w) func();

    for (int b = 0; b < numBatches; ++b) {
        double start_time = omp_get_wtime();
        for (int i = 0; i < repsPerBatch; ++i) func();
        double end_time = omp_get_wtime();
        batchTimes[b] = (end_time - start_time) / repsPerBatch;
        sumTimes      += batchTimes[b];
    }

    avgTime = sumTimes / numBatches;

    double sumSqDiff = 0.0;
    for (int b = 0; b < numBatches; ++b) {
        double diff = batchTimes[b] - avgTime;
        sumSqDiff += diff * diff;
    }
    stdDevTime = (numBatches > 1) ? std::sqrt(sumSqDiff / (numBatches - 1)) : 0.0;
}

// ─────────────────────────────────────────────────────────────
//  runScalingAnalysis
//  Scaling de 1 hasta maxThreads. Calcula Sp, Ep, f_s, f_p.
// ─────────────────────────────────────────────────────────────
void Benchmark::runScalingAnalysis(int maxThreads,
                                    const std::function<void(bool)>& func,
                                    bool fork_join_outside) {
    results.clear();

    double t1_avg    = 0.0;
    double t1_stddev = 0.0;

    for (int p = 1; p <= maxThreads; ++p) {
        double avgTime, stdDevTime;
        runExperiment(p, func, avgTime, stdDevTime, fork_join_outside);

        BenchmarkResult res;
        res.numThreads = p;
        res.avgTime    = avgTime;
        res.stdDevTime = stdDevTime;

        if (p == 1) {
            t1_avg    = avgTime;
            t1_stddev = stdDevTime;

            res.speedup         = 1.0;
            res.speedupError    = 0.0;
            res.efficiency      = 1.0;
            res.efficiencyError = 0.0;
            res.serialFraction   = 1.0; // sin paralelismo → todo serial
            res.parallelFraction = 0.0;
        } else {
            // Speedup: Sp = T1 / Tp
            res.speedup = t1_avg / avgTime;

            // Error de Speedup: σ_Sp = Sp * √( (σ_T1/T1)² + (σ_Tp/Tp)² )
            double rel_err_t1 = (t1_avg > 0) ? t1_stddev / t1_avg : 0.0;
            double rel_err_tp = (avgTime > 0) ? stdDevTime / avgTime : 0.0;
            res.speedupError = res.speedup * std::sqrt(rel_err_t1 * rel_err_t1 +
                                                        rel_err_tp * rel_err_tp);

            // Eficiencia: Ep = Sp / p
            res.efficiency      = res.speedup / p;
            res.efficiencyError = res.speedupError / p;

            // Fracción serial (Amdahl): f_s = (1/Sp - 1/p) / (1 - 1/p)
            double inv_sp = 1.0 / res.speedup;
            double inv_p  = 1.0 / static_cast<double>(p);
            res.serialFraction   = (inv_sp - inv_p) / (1.0 - inv_p);
            res.parallelFraction = 1.0 - res.serialFraction;

            // Clamp para evitar valores fuera de [0,1] por ruido estadístico
            res.serialFraction   = std::max(0.0, std::min(1.0, res.serialFraction));
            res.parallelFraction = std::max(0.0, std::min(1.0, res.parallelFraction));
        }

        results.push_back(res);

        std::cout << "Hilos: " << p
                  << " | T: "  << avgTime << "s (±" << stdDevTime << ")"
                  << " | Sp: " << res.speedup
                  << " | Ep: " << res.efficiency
                  << " | f_s: " << res.serialFraction
                  << " | f_p: " << res.parallelFraction << "\n";
    }
}

// ─────────────────────────────────────────────────────────────
//  saveResultsToFile
//  Formato: Threads AvgTime StdDev Speedup SpeedupErr Efficiency EffErr SerialFrac ParallelFrac
// ─────────────────────────────────────────────────────────────
void Benchmark::saveResultsToFile(const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << "\n";
        return;
    }

    out << "Threads AvgTime StdDevTime Speedup SpeedupError "
           "Efficiency EfficiencyError SerialFraction ParallelFraction\n";

    for (const auto& res : results) {
        out << std::fixed << std::setprecision(6)
            << res.numThreads      << " "
            << res.avgTime         << " "
            << res.stdDevTime      << " "
            << res.speedup         << " "
            << res.speedupError    << " "
            << res.efficiency      << " "
            << res.efficiencyError << " "
            << res.serialFraction  << " "
            << res.parallelFraction << "\n";
    }
    out.close();
    std::cout << "Resultados guardados en: " << filename << "\n";
}

// ─────────────────────────────────────────────────────────────
//  runChunkAnalysis
//  Itera sobre schedules × chunkSizes con hilos fijos.
// ─────────────────────────────────────────────────────────────
void Benchmark::runChunkAnalysis(int numThreads,
                                  std::vector<int> chunkSizes,
                                  std::vector<int> schedules,
                                  const std::function<void(int, int)>& func) {
    omp_set_num_threads(numThreads);
    chunkResults.clear();

    auto scheduleName = [](int s) -> const char* {
        switch (s) {
            case 1:  return "static";
            case 2:  return "dynamic";
            case 3:  return "guided";
            default: return "unknown";
        }
    };

    for (int sched : schedules) {
        for (int chunk : chunkSizes) {
            double avg = 0.0, stddev = 0.0;
            int s = sched, c = chunk;
            runExperimentSimple([&]() { func(s, c); }, avg, stddev);

            ChunkResult cr;
            cr.scheduleType = sched;
            cr.chunkSize    = chunk;
            cr.avgTime      = avg;
            cr.stdDevTime   = stddev;
            chunkResults.push_back(cr);

            std::cout << "Schedule: " << scheduleName(sched)
                      << " | Chunk: "   << chunk
                      << " | T: "       << avg << "s (±" << stddev << ")\n";
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  saveChunkResultsToFile
// ─────────────────────────────────────────────────────────────
void Benchmark::saveChunkResultsToFile(const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << "\n";
        return;
    }

    auto scheduleName = [](int s) -> const char* {
        switch (s) {
            case 1:  return "static";
            case 2:  return "dynamic";
            case 3:  return "guided";
            default: return "unknown";
        }
    };

    out << "Schedule ChunkSize AvgTime StdDevTime\n";
    for (const auto& cr : chunkResults) {
        out << std::left  << std::setw(8) << scheduleName(cr.scheduleType) << " "
            << std::right << std::setw(9) << cr.chunkSize                  << " "
            << std::fixed << std::setprecision(8)
            << cr.avgTime    << " "
            << cr.stdDevTime << "\n";
    }
    out.close();
    std::cout << "Chunk results guardados en: " << filename << "\n";
}

// ─────────────────────────────────────────────────────────────
//  runPrivateVsShared
//  Mide el tiempo de funcPrivate y funcShared con numThreads hilos.
//  serialRefTime se usa para calcular el speedup de cada variante.
// ─────────────────────────────────────────────────────────────
void Benchmark::runPrivateVsShared(int numThreads,
                                    double serialRefTime,
                                    const std::function<void()>& funcPrivate,
                                    const std::function<void()>& funcShared) {
    privateSharedResults.clear();
    omp_set_num_threads(numThreads);

    auto measure = [&](const std::string& label, const std::function<void()>& f) {
        double avg = 0.0, stddev = 0.0;
        runExperimentSimple(f, avg, stddev);

        PrivateSharedResult r;
        r.label       = label;
        r.numThreads  = numThreads;
        r.avgTime     = avg;
        r.stdDevTime  = stddev;
        r.speedupVsSerial = (avg > 0.0 && serialRefTime > 0.0)
                            ? serialRefTime / avg : 0.0;

        privateSharedResults.push_back(r);
        std::cout << "private/shared [" << label << "]"
                  << " | T: " << avg << "s (±" << stddev << ")"
                  << " | Sp_vs_serial: " << r.speedupVsSerial << "\n";
    };

    measure("private", funcPrivate);
    measure("shared",  funcShared);
}

// ─────────────────────────────────────────────────────────────
//  savePrivateSharedToFile
// ─────────────────────────────────────────────────────────────
void Benchmark::savePrivateSharedToFile(const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << "\n";
        return;
    }

    out << "Mode Threads AvgTime StdDevTime SpeedupVsSerial\n";
    for (const auto& r : privateSharedResults) {
        out << std::left  << std::setw(8) << r.label      << " "
            << std::right << std::setw(7) << r.numThreads << " "
            << std::fixed << std::setprecision(8)
            << r.avgTime         << " "
            << r.stdDevTime      << " "
            << r.speedupVsSerial << "\n";
    }
    out.close();
    std::cout << "Private/shared results guardados en: " << filename << "\n";
}
