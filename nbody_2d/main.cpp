#include <iostream>
#include <string>
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include "Benchmark.h"
#include "MetricsCalculator.h"
#include <limits>

using namespace std;

int readInt(const string& msg) {
    int value;

    while (true) {
        cout << msg;

        if (cin >> value) {
            return value;
        }

        cerr << "Error: debe ingresar un numero entero valido.\n";

        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

double readPositiveDouble(const string& msg) {
    double value;

    while (true) {
        cout << msg;

        if (!(cin >> value)) {
            cerr << "Error: debe ingresar un numero real valido.\n";

            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            continue;
        }

        if (value <= 0) {
            cerr << "Error: el numero debe ser mayor que 0.\n";
            continue;
        }

        return value;
    }
}

static void buildSystem(NBodySystem& system, int sys_type, int nParticles, int seed) {
    if (sys_type == 0) {
        system.randomSystem(nParticles, seed);
        system.savePositions("random_system.dat");
    } else if (sys_type == 1) {
        system.binarySystem(seed);
        system.savePositions("binary_system.dat");
    } else if (sys_type == 2) {
        system.diskSystem(nParticles, seed);
        system.savePositions("disk_system.dat");
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
                    int chunkSize, //blocksize
                    int method, //atomic o shared
                    int kernelVariant,
                    int taskType,
                    bool use_barrier) {
    NBodySystem system(G, epsilon);
    buildSystem(system, sys_type, nParticles, seed);

    NBodySimulator simulator(&system, dt);

    const string energyFile = "energy_" + label + ".dat";
    const string trajectoryFile = "trajectories_" + label + ".dat";

    cout << "\n=== Caso " << caseId << ": " << label << " ===" << endl;
    simulator.simulate(steps, energyFile, trajectoryFile, sim_type, syncType, scheduleType, chunkSize, method, kernelVariant, taskType, use_barrier);
}

/*
static void printModeHelp() {
    cout << "Modos disponibles:\n"
         << "  0 -> serial\n"
         << "  1 -> paralelo con omp for\n"
         << "  2 -> paralelo con tasks\n"
         << "  3 -> ejecutar todos los modos\n";
}
*/

static void printModeHelpCUDA() {
    cout << "Modos disponibles:\n"
         << "  0 -> Ejecucion Serial (Baseline Lab 1)\n"
         << "  1 -> Ejecucion CUDA (Simulacion completa)\n"
         << "  2 -> Validacion tolerancias CPU vs GPU (Rol 3)\n"
         << "  3 -> Ejecutar matriz de Benchmarks GPU (Rol 3)\n";
}

int main() {
    int seed;
    int nParticles = 0;
    double dt;
    double G;
    double epsilon;
    int steps;
    int sys_type;
    int mode;
    int scheduleType = 1;
    int chunkSize = 1;
    //int syncType = 0;
    int energyMethod = 0;
    int kernelVariant = 0;
    int blockSize = 256;
    //bool use_barrier = false;

    seed = readInt("Ingrese seed: ");

    sys_type = readInt("Ingrese tipo de sistema, aleatorio (0), binario (1) o disco (2): ");
    while (sys_type < 0 || sys_type > 2) {
        cerr << "Tipo de sistema invalido.\n";
        sys_type = readInt("Ingrese nuevamente: ");
    }

    if (sys_type != 1) {
        nParticles = readInt("Ingrese numero de particulas: ");
        while (nParticles <= 0) {
            cerr << "El numero de particulas debe ser mayor que 0.\n";
            nParticles = readInt("Ingrese nuevamente: ");
        }
    }

    dt = readPositiveDouble("Ingrese dt: ");

    G = readPositiveDouble("Ingrese G: ");

    epsilon = readPositiveDouble("Ingrese epsilon: ");
  

    steps = readInt("Ingrese numero de pasos: ");
    while (steps <= 0) {
        cerr << "El numero de pasos debe ser mayor que 0.\n";
        steps = readInt("Ingrese numero de pasos: ");
    }


    //switchcase con OpenMP
    /*
    printModeHelp();
    mode = readInt("Seleccione modo de simulacion: ");
    while (mode < 0 || mode > 3) {
        cerr << "Modo no valido.\n";
        printModeHelp();
        mode = readInt("Seleccione modo de simulacion: ");
    }

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
    }
    */

    // Switchcase con CUDA y Benchmarks
    printModeHelpCUDA();
    mode = readInt("Seleccione modo de simulacion: ");
    while (mode < 0 || mode > 3) {
        cerr << "Modo no valido.\n";
        printModeHelpCUDA();
        mode = readInt("Seleccione modo de simulacion: ");
    }

    switch(mode) {
        case 0:
            // Serial (Baseline)
            runCase(0, "serial", steps, nParticles, seed, dt, G, epsilon, sys_type, 0, 0, scheduleType, chunkSize, 0, 0, -1, false);
            break;
            
        case 1: {
            // CUDA
            blockSize = readInt("Ingrese tamano de bloque (ej. 256): ");
            energyMethod = readInt("Metodo de energia GPU - Reduccion compartida (0) o AtomicAdd (1): ");
            kernelVariant = readInt("Variante de kernel - Basico (0) o Shared Memory (1): ");
            
            // Usamos sim_type = 3 para indicar CUDA dentro de tu NBodySimulator::simulate
            // Pasamos blockSize en el argumento chunkSize, y gpuMethod en method.
            runCase(1, "cuda", steps, nParticles, seed, dt, G, epsilon, sys_type, 3, 0, scheduleType, blockSize, energyMethod, kernelVariant, -1, false);
            break;
        }
            
        case 2: {
            // Validación CPU vs GPU
            Benchmark bench;
            bench.compareCpuGpu(nParticles);
            break;
        }
            
        case 3: {
            // Matriz de Benchmarks Obligatoria del Laboratorio
            Benchmark bench;
            std::vector<int> N_vals = {256, 512, 1024, 2000};
            std::vector<int> block_vals = {64, 128, 256, 512, 1024};
            
            cout << "Ejecutando matriz de benchmarks. Esto puede tomar unos minutos...\n";
            for(int n : N_vals) {
                // v=0 (básico), v=1 (shared memory)
                for(int v = 0; v <= 1; ++v) { 
                    for(int b : block_vals) {
                        cout << "Midiendo N=" << n << " | Variante=" << v << " | BlockDim=" << b << "\n";
                        bench.benchmarkKernelOnly(n, v, b);
                        bench.benchmarkEndToEnd(n, v, b);
                    }
                }
            }
            bench.saveGpuResultsToFile("benchmark_results.dat");
            break;
        }
    }

    return 0;
}