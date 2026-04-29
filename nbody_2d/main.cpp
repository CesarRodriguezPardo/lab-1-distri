#include <iostream>
#include <string>
#include "NBodySystem.h"
#include "NBodySimulator.h"

using namespace std;

static void buildSystem(NBodySystem& system, int sys_type, int nParticles, int seed) {
    if (sys_type == 0) {
        system.randomSystem(nParticles, seed);
    } else if (sys_type == 1) {
        system.bynarySystem(seed);
    } else if (sys_type == 2) {
        system.diskSystem(nParticles, seed);
    }
}

static void runCase(int caseId,
                    const string& label,
                    int steps,
                    int nParticles,
                    int seed,
                    double dt,
                    double G,
                    double epsilon,
                    int sys_type,
                    int sim_type,
                    int syncType,
                    int scheduleType,
                    int chunkSize) {
    NBodySystem system(G, epsilon);
    buildSystem(system, sys_type, nParticles, seed);

    NBodySimulator simulator(&system, dt);

    const string energyFile = "energy_" + label + ".dat";
    const string trajectoryFile = "trajectories_" + label + ".dat";

    cout << "\n=== Caso " << caseId << ": " << label << " ===" << endl;
    simulator.simulate(steps, energyFile, trajectoryFile, sim_type, syncType, scheduleType, chunkSize);
}

int main() {
    int seed;
    int nParticles;
    double dt;
    double G;
    double epsilon;
    int steps;
    int sys_type;
    int mode;
    int scheduleType = 1;
    int chunkSize = 1;

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

    cout << "Ingrese tipo de sistema, aleatorio (0), binario (1) o disco (2): ";
    cin >> sys_type;

    cout << "Modo de prueba: serial (0), paralelo-critical (1), paralelo-nowait (2), tasks-critical (3), tasks-nowait (4), todos (5): ";
    cin >> mode;

    switch (mode) {
        case 0:
            runCase(0, "serial", steps, nParticles, seed, dt, G, epsilon, sys_type, 0, 0, scheduleType, chunkSize);
            break;
        case 1:
            runCase(1, "parallel_critical", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 1, scheduleType, chunkSize);
            break;
        case 2:
            runCase(2, "parallel_nowait", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 2, scheduleType, chunkSize);
            break;
        case 3:
            runCase(3, "tasks_critical", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, 1, scheduleType, chunkSize);
            break;
        case 4:
            runCase(4, "tasks_nowait", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, 2, scheduleType, chunkSize);
            break;
        case 5:
            runCase(0, "serial", steps, nParticles, seed, dt, G, epsilon, sys_type, 0, 0, scheduleType, chunkSize);
            runCase(1, "parallel_critical", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 1, scheduleType, chunkSize);
            runCase(2, "parallel_nowait", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 2, scheduleType, chunkSize);
            runCase(3, "tasks_critical", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, 1, scheduleType, chunkSize);
            runCase(4, "tasks_nowait", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, 2, scheduleType, chunkSize);
            break;
        default:
            cout << "Modo no valido. Saliendo." << endl;
            return 1;
    }

    return 0;
}