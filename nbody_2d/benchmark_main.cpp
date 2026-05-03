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
    Benchmark bench(5); 
    bench.runScalingAnalysis(4, [&]() {
        system.zeroAccelerations();
        system.computeAccelerations(); // asumiendo que scheduleType=1, chunkSize=10 es default
    });
    bench.saveResultsToFile("scaling_analysis.dat");

    return 0;
}
