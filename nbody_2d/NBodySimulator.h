#ifndef NBODYSIMULATOR_H
#define NBODYSIMULATOR_H

#include "NBodySystem.h"

class NBodySimulator {
private:
    NBodySystem* system;
    double time_step;
public:
    NBodySimulator(NBodySystem* sys, double dt);
    void integrateEuler();
    void calculateEnergy();
    void processBodies();
};

#endif
