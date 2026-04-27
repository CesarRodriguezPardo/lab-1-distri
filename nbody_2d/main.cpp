#include <iostream>
#include "NBodySystem.h"
#include "NBodySimulator.h"
using namespace std;

int main () {

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
    std::string energyFilename = "energy_timeseries_serial.dat";
    NBodySimulator simulator(&system, dt);

    simulator.simulate(steps, energyFilename, sim_type);

}