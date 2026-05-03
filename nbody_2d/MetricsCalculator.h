#ifndef METRICSCALCULATOR_H
#define METRICSCALCULATOR_H

#include "NBodySystem.h"
#include <string>

class MetricsCalculator {
private:
    NBodySystem* system;
    
    double kineticEnergy;
    double potentialEnergy;
    double totalEnergy;
    double momentumX;
    double momentumY;
    double momentumMagnitude;
    double cmX;
    double cmY;
    double rmsRadius;
    double minDistance;
    
public:
    MetricsCalculator(NBodySystem* sys);
    
    // Métodos solicitados
    void calculateEnergy(int method); // 0 = reduce, 1 = atomic
    void calculateMetricsFirstprivate();
    void calculateFinalStateLastprivate();
    
    // Método auxiliar para calcular todas las métricas
    void calculateAllMetrics();
    
    // Exportar a .dat (ej. energy_timeseries.dat)
    void saveMetricsToFile(const std::string& filename, double time);
    
    // Getters
    double getKineticEnergy() const { return kineticEnergy; }
    double getPotentialEnergy() const { return potentialEnergy; }
    double getTotalEnergy() const { return totalEnergy; }
    double getMomentumMagnitude() const { return momentumMagnitude; }
    double getCmX() const { return cmX; }
    double getCmY() const { return cmY; }
    double getRmsRadius() const { return rmsRadius; }
    double getMinDistance() const { return minDistance; }
};

#endif
