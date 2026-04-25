#include <iostream>
#include "NBodySystem.h"
#include "NBodySimulator.h"
using namespace std;

int main () {

    //si ya existe makefile borrar este comentario y el de abajo.
    //para compilar g++ Particle.h Particle.cpp NBodySystem.h NBodySystem.cpp NBodySimulator.h NBodySimulator.cpp main.cpp -o test.exe

    int seed;
    int nParticles;
    double dt;
    double G;
    double epsilon;
    int steps;
    int sim_type;
    int sys_type;

    cout << "Ingrese seed: ";
    cin >> seed;

    cout << "Ingrese numero de particulas: ";
    cin >> nParticles;

    cout << "Ingrese dt: ";
    cin >> dt;

    cout << "Ingrese G: ";
    cin >> G;

    cout << "Ingrese epsilon: ";
    cin >> epsilon;

    cout << "Ingrese numero de pasos: ";
    cin >> steps;

    cout << "Ingrese tipo de simulacion, en serie (0) o en paralelo (1): ";
    cin >> sim_type;

    cout << "Ingrese tipo de sistema, aleatorio (0), binario (1) o disco (2): ";
    cin >> sys_type;

    NBodySystem system(G, epsilon);

    if (sys_type == 0) {
        system.randomSystem(nParticles, seed);
    } else if (sys_type == 1) {
        system.bynarySystem(seed);
    } else if (sys_type == 2) {
        system.diskSystem(nParticles, seed);
    } else {
        cout << "Tipo de sistema no valido. Saliendo." << endl;
        return 1;
    }

    NBodySimulator simulator(&system, dt);

    simulator.simulate(steps);

}