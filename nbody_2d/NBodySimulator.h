#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"
#include <fstream>
#include <iomanip>

class NBodySimulator {
private:
    NBodySystem* system;
    double time_step;
public:
    NBodySimulator(NBodySystem* sys, double dt);
    void integrateEuler();
    void calculateEnergy();
    void processBodies();
    void simulate(int N, double simG, double simEps, double delta_time);
};

#endif
