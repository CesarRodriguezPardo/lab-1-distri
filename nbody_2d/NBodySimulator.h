#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <string>
#include <omp.h>

class NBodySimulator {
private:
    NBodySystem* system;
    double time_step;
public:
    NBodySimulator(NBodySystem* sys, double dt);
    void integrateEuler();
    void integrateEuler(int syncType); //syncType:1 = critical, 2 = nowait
    void integrateEuler(int syncType, bool use_barrier);
    void calculateEnergy(std::ostream &energyFile);
    void calculateEnergy(std::ostream &energyFile, int method, int scheduleType, int chunkSize); //method: 0 = reduce, 1 = atomic
    void calculateEnergy(std::ostream &energyFile, bool use_private);
    void processBodies(std::ostream &energyFile);
    void processBodies(std::ostream &energyFile, int method, int syncType ,int scheduleType, int chunkSize, bool use_barrier);
    void processBodies(std::ostream &energyFile, int taskType,int syncType);
    void simulate(int steps, std::string energyFilename = "energies.dat", std::string trajectoryFilename = "trajectories.dat", int sim_type = 0, 
        int syncType = 0, int scheduleType = 1, int chunkSize = 10, int method = 0, int taskType = -1, bool use_barrier = false);
};

#endif
