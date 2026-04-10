#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include <fstream>
#include <iomanip>
#include <cmath>

class NBodySimulator {
private:
    NBodySystem* system;
    double time_step;
    //std::ofstream energyFile;
public:
    NBodySimulator(NBodySystem* sys, double dt);
    void integrateEuler();
    void calculateEnergy();
    void processBodies();
    void simulate(int steps);
};

#endif
