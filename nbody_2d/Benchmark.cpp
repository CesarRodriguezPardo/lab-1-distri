#include "Benchmark.h"
#include <omp.h>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

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
                // Barrera: garantiza que el timestamp de master precede
                // al trabajo de todas las hebras (evita carrera en la medición)
                #pragma omp barrier

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





//____________________________________________________________________________________
// Benchmarks lab 2: GPU
//____________________________________________________________________________________


// Pruebas CPU vs GPU con tolerancia en coma flotante
void Benchmark::compareCpuGpu(int n_bodies) {
    std::cout << "Iniciando validacion CPU vs GPU para N = " << n_bodies << "...\n";

    // 1. Configurar sistema base
    double G = 1.0;
    double eps = 0.01;
    NBodySystem sys_cpu(G, eps);
    sys_cpu.randomSystem(n_bodies, 42); // Semilla fija documentada
    
    // Clonamos para la GPU
    NBodySystem sys_gpu = sys_cpu; 

    // 2. Calcular en CPU (Baseline Serial)
    sys_cpu.zeroAccelerations();
    sys_cpu.computeAccelerations(); // El del Lab 1

    // 3. Calcular en GPU
    CudaBuffer buffer(n_bodies, sys_gpu.getParticles());
    int blockSize = 256;
    
    // Usamos el lanzador del kernel para la GPU
    launchComputeAccelerationsKernel(
        buffer.getd_mass(), buffer.getd_x(), buffer.getd_y(), 
        buffer.getd_ax(), buffer.getd_ay(), 
        G, eps, n_bodies, blockSize
    );
    
    // Obtenemos las aceleraciones de vuelta a la CPU para comparación
    buffer.retrieveAccelerations(sys_gpu.getParticles());

    // 4. Comparación con Tolerancia (posiblemente requeriría ajuste)
    double rtol = 1e-4;
    double atol = 1e-8;
    bool pass = true;

    const auto& particles_cpu = sys_cpu.getParticles();
    const auto& particles_gpu = sys_gpu.getParticles();

    for (int i = 0; i < n_bodies; ++i) {
        double ax_c = particles_cpu[i].getAX();
        double ax_g = particles_gpu[i].getAX();
        
        double ay_c = particles_cpu[i].getAY();
        double ay_g = particles_gpu[i].getAY();

        // Fórmula matemática exigida: |CPU - GPU| <= atol + rtol * |CPU|
        double diff_x = std::abs(ax_c - ax_g);
        double tol_x = atol + rtol * std::abs(ax_c);

        double diff_y = std::abs(ay_c - ay_g);
        double tol_y = atol + rtol * std::abs(ay_c);

        if (diff_x > tol_x || diff_y > tol_y) {
            std::cerr << "[FALLO] Particula " << i << " excede tolerancia.\n"
                      << "  X -> CPU: " << ax_c << " | GPU: " << ax_g << " | Diff: " << diff_x << "\n"
                      << "  Y -> CPU: " << ay_c << " | GPU: " << ay_g << " | Diff: " << diff_y << "\n";
            pass = false;
            break; // Detenemos en el primer error
        }
    } // Nota: estaría bien poner por cuanto falló, para ajustar la tolerancia si es necesario.

    if (pass) {
        std::cout << "[EXITO] Todas las aceleraciones coinciden dentro de los limites de tolerancia.\n";
    } else {
        std::cout << "[ERROR] La validacion fallo.\n";
    }
}


// Mide el tiempo del kernel de aceleración excluyendo transferencias
void Benchmark::benchmarkKernelOnly(int n_bodies, int variant, int blockDim) {
    NBodySystem sys(1.0, 0.01);
    sys.randomSystem(n_bodies, 42);
    
    // El buffer se crea AFUERA del temporizador (excluye H2D)
    CudaBuffer buffer(n_bodies, sys.getParticles());

    // Obtenemos el tiempo serial de la CPU para calcular el speedup
    double cpuAvgTime = 0.0, cpuStdDev = 0.0;
    auto cpuFunc = [&]() { sys.computeAccelerations(); };
    runExperimentSimple(cpuFunc, cpuAvgTime, cpuStdDev);

    std::vector<double> times(numRepetitions);
    double totalTime = 0.0;

    for (int i = 0; i < numRepetitions; ++i) {
        // Inicio de temporización
        auto start = std::chrono::steady_clock::now();

        if (variant == 0) {
            launchComputeAccelerationsKernel(
                buffer.getd_mass(), buffer.getd_x(), buffer.getd_y(), 
                buffer.getd_ax(), buffer.getd_ay(), 
                1.0, 0.01, n_bodies, blockDim
            );
        } else {
            launchComputeAccelerationsKernelShared(
                buffer.getd_mass(), buffer.getd_x(), buffer.getd_y(), 
                buffer.getd_ax(), buffer.getd_ay(), 
                1.0, 0.01, n_bodies, blockDim
            );
        }

        // Fin de temporización
        auto end = std::chrono::steady_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        times[i] = elapsed.count();
        totalTime += times[i];
    }

    // Cálculos estadísticos
    double avgTime = totalTime / numRepetitions;
    double variance = 0.0;
    for (double t : times) {
        variance += (t - avgTime) * (t - avgTime);
    }
    double stdDevTime = std::sqrt(variance / numRepetitions);

    // Guardar resultados
    GpuBenchmarkResult res;
    res.n_bodies = n_bodies;
    res.variant = variant;
    res.blockDim = blockDim;
    res.measureType = "kernel-only";
    res.avgTime = avgTime;
    res.stdDevTime = stdDevTime;
    res.speedup = cpuAvgTime / avgTime;
    res.cpuStdDev = cpuStdDev;
    // Propagación de error del speedup según fórmula (4):
    // σ_S = S · sqrt( (σ_Tcpu/T_cpu)² + (σ_Tgpu/T_gpu)² )
    if (cpuAvgTime > 0.0 && avgTime > 0.0) {
        double relCpu = cpuStdDev / cpuAvgTime;
        double relGpu = stdDevTime / avgTime;
        res.speedupError = res.speedup * std::sqrt(relCpu * relCpu + relGpu * relGpu);
    } else {
        res.speedupError = 0.0;
    }

    gpuResults.push_back(res);
}

// Mide el tiempo del paso completo, incluyendo transferencias H2D y D2H
void Benchmark::benchmarkEndToEnd(int n_bodies, int variant, int blockDim) {
    NBodySystem sys(1.0, 0.01);
    sys.randomSystem(n_bodies, 42);

    // Obtenemos el tiempo serial de la CPU para calcular el speedup
    double cpuAvgTime = 0.0, cpuStdDev = 0.0;
    auto cpuFunc = [&]() { sys.computeAccelerations(); };
    runExperimentSimple(cpuFunc, cpuAvgTime, cpuStdDev);

    std::vector<double> times(numRepetitions);
    double totalTime = 0.0;

    for (int i = 0; i < numRepetitions; ++i) {
        // Inicio de temporización (Incluye TODO el paso temporal)
        auto start = std::chrono::steady_clock::now();

        // 1. Transferencia Host to Device (H2D) encapsulada en el constructor
        CudaBuffer buffer(n_bodies, sys.getParticles());

        // 2. Cómputo del Kernel + Sincronización
        if (variant == 0) {
            launchComputeAccelerationsKernel(
                buffer.getd_mass(), buffer.getd_x(), buffer.getd_y(), 
                buffer.getd_ax(), buffer.getd_ay(), 
                1.0, 0.01, n_bodies, blockDim
            );
        } else {
            launchComputeAccelerationsKernelShared(
                buffer.getd_mass(), buffer.getd_x(), buffer.getd_y(), 
                buffer.getd_ax(), buffer.getd_ay(), 
                1.0, 0.01, n_bodies, blockDim
            );
        }

        // 3. Transferencia Device to Host (D2H) encapsulada en la recuperación
        buffer.retrieveAccelerations(sys.getParticles());

        // Fin de temporización
        auto end = std::chrono::steady_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        times[i] = elapsed.count();
        totalTime += times[i];
    }

    // Cálculos estadísticos
    double avgTime = totalTime / numRepetitions;
    double variance = 0.0;
    for (double t : times) {
        variance += (t - avgTime) * (t - avgTime);
    }
    double stdDevTime = std::sqrt(variance / numRepetitions);

    // Guardar resultados
    GpuBenchmarkResult res;
    res.n_bodies = n_bodies;
    res.variant = variant;
    res.blockDim = blockDim;
    res.measureType = "end-to-end";
    res.avgTime = avgTime;
    res.stdDevTime = stdDevTime;
    res.speedup = cpuAvgTime / avgTime;
    res.cpuStdDev = cpuStdDev;
    // Propagación de error del speedup según fórmula (4):
    // σ_S = S · sqrt( (σ_Tcpu/T_cpu)² + (σ_Tgpu/T_gpu)² )
    if (cpuAvgTime > 0.0 && avgTime > 0.0) {
        double relCpu = cpuStdDev / cpuAvgTime;
        double relGpu = stdDevTime / avgTime;
        res.speedupError = res.speedup * std::sqrt(relCpu * relCpu + relGpu * relGpu);
    } else {
        res.speedupError = 0.0;
    }

    gpuResults.push_back(res);
}


// Para exportar datos del clúster a .dat
void Benchmark::saveGpuResultsToFile(const std::string& filename) {
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        std::cerr << "Error al abrir el archivo " << filename << " para escribir los resultados GPU.\n";
        return;
    }

    // Cabecera del archivo de datos
    outFile << std::setw(10) << "N_Bodies" 
            << std::setw(10) << "Variant" 
            << std::setw(12) << "BlockDim" 
            << std::setw(15) << "MeasureType" 
            << std::setw(15) << "AvgTime(s)" 
            << std::setw(15) << "StdDev(s)" 
            << std::setw(15) << "Speedup"
            << std::setw(15) << "CpuStdDev(s)"
            << std::setw(15) << "SpeedupError" << "\n";

    for (const auto& res : gpuResults) {
        outFile << std::setw(10) << res.n_bodies
                << std::setw(10) << res.variant
                << std::setw(12) << res.blockDim
                << std::setw(15) << res.measureType
                << std::setw(15) << res.avgTime
                << std::setw(15) << res.stdDevTime
                << std::setw(15) << res.speedup
                << std::setw(15) << res.cpuStdDev
                << std::setw(15) << res.speedupError << "\n";
    }

    outFile.close();
    std::cout << "Resultados GPU guardados exitosamente en: " << filename << "\n";
}
