#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include <fstream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <string>

class NBodySimulator {
private:
    NBodySystem* system;
    double time_step;
public:
    NBodySimulator(NBodySystem* sys, double dt);
    void integrateEuler();
    void calculateEnergy(std::ofstream &energyFile);
    void processBodies(std::ofstream &energyFile, int sim_type);
    void simulate(int steps, std::string filename, int sim_type);
};

#endif
