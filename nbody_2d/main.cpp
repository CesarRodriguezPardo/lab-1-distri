#include <iostream>
#include <string>
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include "Benchmark.h"
#include "MetricsCalculator.h"

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
                    int chunkSize,
                    int method,
                    int taskType,
                    bool use_barrier) {
    NBodySystem system(G, epsilon);
    buildSystem(system, sys_type, nParticles, seed);

    NBodySimulator simulator(&system, dt);

    const string energyFile = "energy_" + label + ".dat";
    const string trajectoryFile = "trajectories_" + label + ".dat";

    cout << "\n=== Caso " << caseId << ": " << label << " ===" << endl;
    simulator.simulate(steps, energyFile, trajectoryFile, sim_type, syncType, scheduleType, chunkSize, method, taskType, use_barrier);
}

static void printModeHelp() {
    cout << "Modos disponibles:\n"
         << "  0 -> serial\n"
         << "  1 -> paralelo con omp for\n"
         << "  2 -> paralelo con tasks\n"
         << "  3 -> ejecutar todos los modos\n";
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
    int syncType = 0;
    int method = 0;
    bool use_barrier = false;

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

    printModeHelp();
    cout << "Modo de prueba: ";
    cin >> mode;

    switch (mode) {
        case 0:
            runCase(0, "serial", steps, nParticles, seed, dt, G, epsilon, sys_type, 0, 0, scheduleType, chunkSize, 0, -1, false);
            break;
        case 1:
            cout << "Metodo de energia: reduction (0), atomic (1), private (2): ";
            cin >> method;
            cout << "Sincronizacion de integracion: critical (1), nowait (2): ";
            cin >> syncType;
            if (syncType == 2) {
                int barrierChoice;
                cout << "Usar barrera explicita tras nowait? si (1), no (0): ";
                cin >> barrierChoice;
                use_barrier = (barrierChoice == 1);
            }
            cout << "Schedule para omp for: static (1), dynamic (2), guided (3), auto (4): ";
            cin >> scheduleType;
            cout << "Chunk size: ";
            cin >> chunkSize;
            runCase(1, "parallel_for", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, syncType, scheduleType, chunkSize, method, -1, use_barrier);
            break;
        case 2:
            cout << "Sincronizacion de integracion: critical (1), nowait (2): ";
            cin >> syncType;
            runCase(2, "tasks", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, syncType, scheduleType, chunkSize, 0, 0, false);
            break;
        case 3:
            runCase(0, "serial", steps, nParticles, seed, dt, G, epsilon, sys_type, 0, 0, scheduleType, chunkSize, 0, -1, false);
            runCase(1, "parallel_for_reduction", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 2, 1, 16, 0, -1, false);
            runCase(2, "parallel_for_atomic", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 2, 1, 16, 1, -1, false);
            runCase(3, "parallel_for_private", steps, nParticles, seed, dt, G, epsilon, sys_type, 1, 2, 1, 16, 2, -1, false);
            runCase(4, "tasks_private", steps, nParticles, seed, dt, G, epsilon, sys_type, 2, 2, scheduleType, chunkSize, 0, 0, false);
            break;
        default:
            cout << "Modo no valido. Saliendo." << endl;
            return 1;
    }

    return 0;
}