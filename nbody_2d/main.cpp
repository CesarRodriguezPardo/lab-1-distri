#include "NBodySystem.h"
#include "NBodySimulator.h"

int main () {

    double G = 1.0;
    double epsilon = 0.1;
    double dt = 0.01;
    int seed = 31; //valor de ejemplo
    int amountOfP = 3;
    int steps = 100;

    NBodySystem system(G, epsilon);

    system.generateParticles(amountOfP, seed);

    NBodySimulator simulator(&system, dt);

    simulator.simulate(steps);



    /*    Particle p1(1.0, 0.0, 0.0);
    Particle p2(1.0, 1.0, 0.0);
    system.addParticle(p1);
    system.addParticle(p2);
    */

    /*
    NBodySimulator simulator(&system, dt);
    for (int step = 0; step < 20; step++) {
        simulator.integrateEuler();
        if (step % 2 == 0) {
            std::cout << "Paso " << step << " completado." << std::endl;
        }
    }
    */
}