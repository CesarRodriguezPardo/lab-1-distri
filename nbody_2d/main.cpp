#include "NBodySystem.h"
#include "NBodySimulator.h"

int main () {

    //si ya existe makefile borrar este comentario y el de abajo.
    //para compilar g++ Particle.h Particle.cpp NBodySystem.h NBodySystem.cpp NBodySimulator.h NBodySimulator.cpp main.cpp -o test.exe

    int seed = 31;
    int nParticles = 100;
    double dt = 0.01; //que tan grande va a ser el movimiento de las particulas en cada paso
    double G = 1.0;
    double epsilon = 0.05;
    int steps = 1000;

    NBodySystem system(G, epsilon);

    //system.randomSystem(nParticles, seed);
    //system.bynarySystem(seed);
    system.diskSystem(nParticles, seed);

    NBodySimulator simulator(&system, dt);

    simulator.simulate(steps);

}