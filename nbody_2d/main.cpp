#include <iostream>
#include "NBodySystem.h"
#include "NBodySimulator.h"

int main () {

    double G = 1.0;
    double epsilon = 0.1;
    double dt = 0.01;

    NBodySystem system(G, epsilon);

    Particle p1(1.0, 0.0, 0.0);
    Particle p2(1.0, 1.0, 0.0);
    system.addParticle(p1);
    system.addParticle(p2);

    NBodySimulator simulator(&system, dt);
    for (int step = 0; step < 20; step++) {
        simulator.integrateEuler();
        if (step % 2 == 0) {
            std::cout << "Paso " << step << " completado." << std::endl;
        }
    }
}