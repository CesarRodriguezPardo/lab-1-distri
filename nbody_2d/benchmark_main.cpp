#include <iostream>
#include "NBodySystem.h"
#include "MetricsCalculator.h"
#include "Benchmark.h"

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

    std::cout << "\nEjecutando Benchmark de computeAccelerations (1 a 4 hilos, 5 repeticiones)...\n";
    Benchmark bench(20); // 20 repeticiones por lote
    bench.runScalingAnalysis(4, [&](bool inside_parallel) {
        // Zeroing has to be done carefully inside parallel. 
        // We will do it master-only or single-only?
        // Actually, zeroAccelerations sets all ax, ay to 0. It is O(N) and fast. 
        // We can let the master do it or parallelize it. 
        // For benchmarking purposes, zeroing can be parallelized or master-only.
        #pragma omp single
        system.zeroAccelerations();
        
        system.computeAccelerations(1, 0, inside_parallel); // scheduleType=1 (static)
    }, true);
    bench.saveResultsToFile("scaling_analysis.dat");

    return 0;
}
