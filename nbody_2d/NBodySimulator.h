#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include "Integrator.h"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <string>
#include <omp.h>

class NBodySimulator {
private:
    NBodySystem* system;
    double       time_step;
    Integrator   integrator; // responsable de la integración de movimiento

public:
    NBodySimulator(NBodySystem* sys, double dt);

    // ── Integración (delega a Integrator) ───────────────────────
    void integrateEuler();
    void integrateEuler(int syncType);
    void integrateEuler(int syncType, bool use_barrier);

    // ── Cálculo de energía ──────────────────────────────────────
    void calculateEnergy(std::ostream& energyFile);
    void calculateEnergy(std::ostream& energyFile, int method, int scheduleType, int chunkSize);
    void calculateEnergy(std::ostream& energyFile, bool use_private);

    // ── Paso completo: aceleraciones + integración + energía ─────
    void processBodies(std::ostream& energyFile);
    void processBodies(std::ostream& energyFile, int method, int syncType,
                       int scheduleType, int chunkSize, bool use_barrier);
    void processBodies(std::ostream& energyFile, int taskType, int syncType);

    // ── Simulación completa de N pasos ───────────────────────────
    void simulate(int steps,
                  std::string energyFilename      = "energies.dat",
                  std::string trajectoryFilename  = "trajectories.dat",
                  int  sim_type    = 0,
                  int  syncType    = 0,
                  int  scheduleType = 1,
                  int  chunkSize   = 10,
                  int  method      = 0,
                  int  taskType    = -1,
                  bool use_barrier = false);
};

#endif
