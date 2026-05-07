// ─────────────────────────────────────────────────────────────
//  benchmark_main.cpp
//  Punto de entrada del módulo de benchmarking del simulador N-Body.
//
//  Benchmarks ejecutados:
//   A) Escalabilidad parallel-for  (1→maxThreads, computeAccelerations)
//   B) Escalabilidad tasks         (1→maxThreads, computeAccelerations)
//   C) private vs shared           (numThreads fijo, calculateEnergy)
//   D) Chunk × Schedule analysis   (numThreads fijo, computeAccelerations)
//   E) Full simulation             (1000 pasos por modo: serial/parallel/tasks)
// ─────────────────────────────────────────────────────────────
#include <iostream>
#include <sstream>
#include <chrono>
#include <omp.h>
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include "Benchmark.h"

// ─── Constantes de configuración del benchmark ────────────────
static const int    N_PARTICLES   = 1000;   // partículas en el sistema
static const int    BENCH_SEED    = 42;     // semilla reproducible
static const int    N_REPS        = 20;     // repeticiones por lote
static const int    FULL_SIM_STEPS = 1000; // pasos de la simulación final (rango 500–2000)

int main() {
    // Detectar número máximo de hilos disponibles en el sistema
    int maxThreads = omp_get_max_threads();
    std::cout << "=== Benchmark N-Body 2D ===\n"
              << "Partículas : " << N_PARTICLES   << "\n"
              << "Hilos max  : " << maxThreads    << "\n"
              << "Repeticiones por lote: " << N_REPS << "\n\n";

    // ─── Sistema compartido por todos los benchmarks ───────────
    NBodySystem system(1.0, 0.05);
    system.randomSystem(N_PARTICLES, BENCH_SEED);

    NBodySimulator simulator(&system, 0.01);

    // Stream nulo para descartar I/O de energía durante el benchmark
    std::ostringstream devNull;

    // ═══════════════════════════════════════════════════════════
    //  BENCHMARK A — Escalabilidad parallel for
    //  Mide solo computeAccelerations(scheduleType, chunkSize)
    //  con schedule static y chunk automático.
    // ═══════════════════════════════════════════════════════════
    std::cout << "──────────────────────────────────────────────\n"
              << "BENCHMARK A: Escalabilidad parallel-for\n"
              << "──────────────────────────────────────────────\n";

    Benchmark benchParallel(N_REPS);
    benchParallel.runScalingAnalysis(maxThreads,
        [&](bool /*inside_parallel*/) {
            // Benchmarkeamos solo el núcleo de cálculo de fuerzas
            system.computeAccelerations(1 /*static*/, 0 /*chunk auto*/);
        }, false);
    benchParallel.saveResultsToFile("scaling_parallel.dat");

    // ═══════════════════════════════════════════════════════════
    //  BENCHMARK B — Escalabilidad tasks
    //  Mide solo computeAccelerations(taskType=0)
    // ═══════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────\n"
              << "BENCHMARK B: Escalabilidad tasks\n"
              << "──────────────────────────────────────────────\n";

    Benchmark benchTasks(N_REPS);
    benchTasks.runScalingAnalysis(maxThreads,
        [&](bool /*inside_parallel*/) {
            // taskType=0 → single + omp task + taskwait
            system.computeAccelerations(0 /*taskType*/);
        }, false);
    benchTasks.saveResultsToFile("scaling_tasks.dat");

    // ═══════════════════════════════════════════════════════════
    //  BENCHMARK C — private vs shared
    //  Compara calculateEnergy con variables privadas vs compartidas.
    //  Usa numThreads fijo = maxThreads.
    //  Obtiene el tiempo serial de referencia (1 hilo).
    // ═══════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────\n"
              << "BENCHMARK C: private vs shared (calculateEnergy)\n"
              << "──────────────────────────────────────────────\n";

    // Tiempo serial de referencia: calculateEnergy con 1 hilo
    omp_set_num_threads(1);
    double t_serial_energy_avg = 0.0, t_serial_energy_std = 0.0;
    {
        Benchmark tmpSerial(N_REPS);
        // Usamos runExperiment internamente vía runScalingAnalysis con maxThreads=1
        // Pero es más directo: un warmup manual + medición simple
        int warmReps = 3, measureReps = N_REPS, nBatches = 5;
        for (int w = 0; w < warmReps; ++w)
            simulator.calculateEnergy(static_cast<std::ostream&>(devNull), false /*shared*/);

        std::vector<double> batchTimes(nBatches);
        for (int b = 0; b < nBatches; ++b) {
            double t0 = omp_get_wtime();
            for (int i = 0; i < measureReps; ++i)
                simulator.calculateEnergy(static_cast<std::ostream&>(devNull), false);
            double t1 = omp_get_wtime();
            batchTimes[b] = (t1 - t0) / measureReps;
            t_serial_energy_avg += batchTimes[b];
        }
        t_serial_energy_avg /= nBatches;
        for (int b = 0; b < nBatches; ++b) {
            double d = batchTimes[b] - t_serial_energy_avg;
            t_serial_energy_std += d * d;
        }
        t_serial_energy_std = std::sqrt(t_serial_energy_std / (nBatches - 1));
        std::cout << "Referencia serial (1 hilo): T=" << t_serial_energy_avg
                  << "s (±" << t_serial_energy_std << ")\n";
    }

    int fixedThreadsPrivShared = maxThreads;
    omp_set_num_threads(fixedThreadsPrivShared);

    Benchmark benchPrivShared(N_REPS);
    benchPrivShared.runPrivateVsShared(
        fixedThreadsPrivShared,
        t_serial_energy_avg,
        // funcPrivate → calculateEnergy(bool use_private = true)
        [&]() { simulator.calculateEnergy(static_cast<std::ostream&>(devNull), true); },
        // funcShared  → calculateEnergy(bool use_private = false)
        [&]() { simulator.calculateEnergy(static_cast<std::ostream&>(devNull), false); }
    );
    benchPrivShared.savePrivateSharedToFile("private_vs_shared.dat");

    // ═══════════════════════════════════════════════════════════
    //  BENCHMARK D — Chunk × Schedule analysis
    //  Varía el tipo de schedule y el tamaño de chunk con hilos fijos.
    // ═══════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────\n"
              << "BENCHMARK D: Chunk × Schedule analysis\n"
              << "──────────────────────────────────────────────\n";

    int fixedThreadsChunk = maxThreads;
    std::vector<int> chunkSizes = {1, 8, 32, 128, 512};
    std::vector<int> schedules  = {1, 2, 3};  // 1=static, 2=dynamic, 3=guided

    Benchmark benchChunk(N_REPS);
    benchChunk.runChunkAnalysis(fixedThreadsChunk, chunkSizes, schedules,
        [&](int sched, int chunk) {
            system.computeAccelerations(sched, chunk);
        });
    benchChunk.saveChunkResultsToFile("chunk_schedule.dat");

    // ═══════════════════════════════════════════════════════════
    //  BENCHMARK E — Full Simulation
    //  FULL_SIM_STEPS pasos completos por modo (serial/parallel/tasks).
    //  Demuestra que el sistema evoluciona correctamente y mide el tiempo total.
    //  Nota: este benchmark SÍ incluye integración + I/O descartada (devNull).
    // ═══════════════════════════════════════════════════════════
    std::cout << "\n──────────────────────────────────────────────\n"
              << "BENCHMARK E: Full Simulation (" << FULL_SIM_STEPS << " pasos)\n"
              << "──────────────────────────────────────────────\n";

    // Reiniciar el sistema para la simulación final
    NBodySystem systemFull(1.0, 0.05);
    systemFull.randomSystem(N_PARTICLES, BENCH_SEED);
    NBodySimulator simFull(&systemFull, 0.01);
    std::ostringstream devNull2;

    auto runFullSim = [&](const std::string& label, auto fn) {
        std::cout << " -> " << label << "... " << std::flush;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int s = 0; s < FULL_SIM_STEPS; ++s) fn();
        auto t1 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = t1 - t0;
        std::cout << elapsed.count() << " s ("
                  << elapsed.count() / FULL_SIM_STEPS * 1000.0 << " ms/paso)\n";
        return elapsed.count();
    };

    omp_set_num_threads(maxThreads);

    double tSerial = runFullSim("SERIAL  ", [&]() {
        simFull.processBodies(static_cast<std::ostream&>(devNull2));
    });

    double tParallel = runFullSim("PARALLEL", [&]() {
        simFull.processBodies(static_cast<std::ostream&>(devNull2), 1 /*method*/, 2 /*nowait*/, 1 /*static*/, 0 /*chunk*/, false);
    });

    double tTasks = runFullSim("TASKS   ", [&]() {
        simFull.processBodies(static_cast<std::ostream&>(devNull2), 0 /*taskType*/, 2 /*nowait*/);
    });

    std::cout << "\nResumen full simulation:\n"
              << "  Speedup parallel vs serial: " << tSerial / tParallel << "x\n"
              << "  Speedup tasks vs serial   : " << tSerial / tTasks    << "x\n";

    std::cout << "\n=== Benchmark completo. Archivos generados: ===\n"
              << "  scaling_parallel.dat\n"
              << "  scaling_tasks.dat\n"
              << "  private_vs_shared.dat\n"
              << "  chunk_schedule.dat\n";

    return 0;
}
