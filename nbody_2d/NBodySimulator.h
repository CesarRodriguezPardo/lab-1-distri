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
    void integrateEuler(int syncType); //syncType: 1 = critical, 2 = nowait
    void calculateEnergy(std::ostream &energyFile);
    void calculateEnergy(std::ostream &energyFile, int sim_type); //sim_type: 0 = serial, 1 = parallel, 2 = tasks
    void processBodies(std::ostream &energyFile, int sim_type, int syncType ,int scheduleType, int chunkSize);
    void simulate(int steps, std::string energyFilename = "energies.dat", std::string trajectoryFilename = "trajectories.dat", int sim_type = 0, int syncType = 0, int scheduleType = 1, int chunkSize = 10);
};

#endif
