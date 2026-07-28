#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include "Integrator.h"
#include "CudaBuffer.h" // <-- NUEVO: Incluimos la clase del Rol 2
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

    void integrateEuler();
    void integrateEuler(int syncType);
    void integrateEuler(int syncType, bool use_barrier);

    void calculateEnergy(std::ostream& energyFile);
    void calculateEnergy(std::ostream& energyFile, int method, int scheduleType, int chunkSize);
    void calculateEnergy(std::ostream& energyFile, bool use_private);


    void processBodies(std::ostream& energyFile);
    void processBodies(std::ostream& energyFile, int method, int syncType,
                       int scheduleType, int chunkSize, bool use_barrier);
    void processBodies(std::ostream& energyFile, int taskType, int syncType);


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


    void stepEulerGpu(CudaBuffer* buffer);

    // Delega a la métrica por defecto
    void calculateEnergyGpu(CudaBuffer* buffer, std::ostream& energyFile);
    // Calcula la energía definiendo variante (0 = shared memory, 1 = atomicAdd)
    void calculateEnergyGpu(int method, CudaBuffer* buffer, std::ostream& energyFile);
};

#endif