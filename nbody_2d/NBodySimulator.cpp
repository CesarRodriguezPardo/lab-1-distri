#include "NBodySimulator.h"
#include <memory>

#define CUDA_CHECK_THROW(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw std::runtime_error("Error CUDA en cudaMalloc: " + \
                std::string(cudaGetErrorString(err))); \
        } \
    } while (0)


extern void launchKineticAtomic(CudaBuffer* buf, double* d_K, int N, int blockSize);
extern void launchPotentialAtomic(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize);
extern void launchKineticShared(CudaBuffer* buf, double* d_K, int N, int blockSize);
extern void launchPotentialShared(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize);

extern void launchComputeAccelerationsKernel(
    const double* d_mass, const double* d_x, const double* d_y, 
    double* d_ax, double* d_ay, double G, double eps, int N, int blockSize
);

extern void launchComputeAccelerationsKernelShared(
    const double* d_mass, const double* d_x, const double* d_y, 
    double* d_ax, double* d_ay, double G, double eps, int N, int blockSize
);



NBodySimulator::NBodySimulator(NBodySystem* sys, double dt)
    : system(sys), time_step(dt), integrator(sys, dt) {}
      

// ─────────────────────────────────────────────────────────────
//  integrateEuler — delegaciones a la clase Integrator
//  La lógica completa vive en Integrator.cpp para mantener
//  separación de responsabilidades.
// ─────────────────────────────────────────────────────────────
void NBodySimulator::integrateEuler() {
    integrator.integrateEuler();
}

void NBodySimulator::integrateEuler(int syncType) {
    integrator.integrateEuler(syncType);
}

void NBodySimulator::integrateEuler(int syncType, bool use_barrier) {
    integrator.integrateEuler(syncType, use_barrier);
}

void NBodySimulator::stepEulerGpu(CudaBuffer* buffer){
    integrator.integrateEulerGpu(buffer);
}

void NBodySimulator::calculateEnergy(std::ostream &energyFile){
    auto& particles = system->getParticles();
    int n = particles.size();
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    double G = system->getG_const();
    double eps = system->getEps();

    for (int i = 0; i < n; ++i){
        double m = particles[i].getMass();
        double vx = particles[i].getVX();
        double vy = particles[i].getVY();
        double xi = particles[i].getX();
        double yi = particles[i].getY();

            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
        for (int j = i + 1; j < n; ++j) {
            double dx = particles[j].getX() - xi;
            double dy = particles[j].getY() - yi;
            
            // Usamos el mismo suavizado 'eps' que en las aceleraciones
            
            double r = sqrt(dx * dx + dy * dy + eps * eps);
            potentialEnergy -= (G * m * particles[j].getMass()) / r;
        }
    }
    double totalEnergy = kineticEnergy + potentialEnergy;

    energyFile << std::fixed << std::setprecision(8) 
            << kineticEnergy << " \t " 
            << potentialEnergy << " \t " 
            << totalEnergy << "\n";
}

void NBodySimulator::calculateEnergyGpu(int method, CudaBuffer* buffer, std::ostream &energyFile) {
    int N = system->getCount();
    double G = system->getG_const();
    double eps = system->getEps();
    int blockSize = 256; 

    double* ptr_K = nullptr;
    double* ptr_U = nullptr;

    CUDA_CHECK_THROW(cudaMalloc((void**)&ptr_K, sizeof(double)));
    std::unique_ptr<double, decltype(&cudaFree)> d_total_K(ptr_K, cudaFree);

    CUDA_CHECK_THROW(cudaMalloc((void**)&ptr_U, sizeof(double)));
    std::unique_ptr<double, decltype(&cudaFree)> d_total_U(ptr_U, cudaFree); 


    CUDA_CHECK(cudaMemset(d_total_K.get(), 0, sizeof(double)));
    CUDA_CHECK(cudaMemset(d_total_U.get(), 0, sizeof(double)));

    if (method == 1) {
        launchKineticAtomic(buffer, d_total_K.get(), N, blockSize);
        launchPotentialAtomic(buffer, d_total_U.get(), N, G, eps, blockSize);
    } else {
        launchKineticShared(buffer, d_total_K.get(), N, blockSize);
        launchPotentialShared(buffer, d_total_U.get(), N, G, eps, blockSize);
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    double h_total_K, h_total_U;
    CUDA_CHECK(cudaMemcpy(&h_total_K, d_total_K.get(), sizeof(double), cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(&h_total_U, d_total_U.get(), sizeof(double), cudaMemcpyDeviceToHost));

    double E_total = h_total_K + h_total_U;
    
    // Guardamos en el archivo .dat para cumplir con la documentación de deriva
    energyFile << std::fixed << std::setprecision(8) 
               << h_total_K << " \t " 
               << h_total_U << " \t " 
               << E_total << "\n";
}


omp_sched_t getScheduleFromSimInt(int type) {
    switch (type) {
        case 1: return omp_sched_static;
        case 2: return omp_sched_dynamic;
        case 3: return omp_sched_guided;
        case 4: return omp_sched_auto;
        default: return omp_sched_static; // Default seguro
    }
}

void NBodySimulator::calculateEnergy(std::ostream &energyFile, int method, int scheduleType, int chunkSize) {
    auto& particles = system->getParticles();
    int n = particles.size();
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    double totalEnergy = 0.0;
    double G = system->getG_const();
    double eps = system->getEps();

    omp_set_schedule(getScheduleFromSimInt(scheduleType), chunkSize);

    if (method == 0) {
        // Paralelizacion con reducción
        #pragma omp parallel for reduction(+:kineticEnergy, potentialEnergy) schedule(runtime)
        for (int i = 0; i < n; ++i){
            double m = particles[i].getMass();
            double vx = particles[i].getVX();
            double vy = particles[i].getVY();
            double xi = particles[i].getX();
            double yi = particles[i].getY();

            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
            for (int j = i + 1; j < n; ++j) {
                double dx = particles[j].getX() - xi;
                double dy = particles[j].getY() - yi;
                
                // Usamos el mismo suavizado 'eps' que en las aceleraciones
                
                double r = sqrt(dx * dx + dy * dy + eps * eps);
                potentialEnergy -= (G * m * particles[j].getMass()) / r;
            }
        }
        totalEnergy = kineticEnergy + potentialEnergy;

    
    } else if (method == 1) {
        // Paralelizacion con atómicos
        #pragma omp parallel for schedule(runtime) shared(kineticEnergy, potentialEnergy)
        for (int i = 0; i < n; ++i){
            double m = particles[i].getMass();
            double vx = particles[i].getVX();
            double vy = particles[i].getVY();
            double xi = particles[i].getX();
            double yi = particles[i].getY();
            #pragma omp atomic
            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
            for (int j = i + 1; j < n; ++j) {
                double dx = particles[j].getX() - xi;
                double dy = particles[j].getY() - yi;

                double r = sqrt(dx * dx + dy * dy + eps * eps);
                #pragma omp atomic
                potentialEnergy -= (G * m * particles[j].getMass()) / r;
            }
        }
        totalEnergy = kineticEnergy + potentialEnergy;
        
    }
    energyFile << std::fixed << std::setprecision(8) 
                << kineticEnergy << " \t " 
                << potentialEnergy << " \t " 
                << totalEnergy << "\n";
}


void NBodySimulator::calculateEnergy(std::ostream &energyFile, bool use_private) {
    auto& particles = system->getParticles();
    int n = particles.size();
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    double G = system->getG_const();
    double eps = system->getEps();
    
    
    if(use_private) {
        double r, dx, dy, m, vx, vy, xi, yi;
        // Paralelizacion con atomicos y con variables privadas
        #pragma omp parallel for schedule(dynamic, 16) private(r, dx, dy, m, vx, vy, xi, yi) reduction(+:kineticEnergy, potentialEnergy)
        for (int i = 0; i < n; ++i){
            m = particles[i].getMass();
            vx = particles[i].getVX();
            vy = particles[i].getVY();
            xi = particles[i].getX();
            yi = particles[i].getY();
            #pragma omp atomic
            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
            for (int j = i + 1; j < n; ++j) {
                dx = particles[j].getX() - xi;
                dy = particles[j].getY() - yi;

                r = sqrt(dx * dx + dy * dy + eps * eps);
                #pragma omp atomic
                potentialEnergy -= (G * m * particles[j].getMass()) / r;
            }
        }
    } else {
        // Paralelizacion con atomicos y compartiendo variables
         #pragma omp parallel for schedule(dynamic, 16) shared(kineticEnergy, potentialEnergy)
         for (int i = 0; i < n; ++i){
            double m = particles[i].getMass();
            double vx = particles[i].getVX();
            double vy = particles[i].getVY();
            double xi = particles[i].getX();
            double yi = particles[i].getY();
            #pragma omp atomic
            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
            for (int j = i + 1; j < n; ++j) {
                double dx = particles[j].getX() - xi;
                double dy = particles[j].getY() - yi;

                double r = sqrt(dx * dx + dy * dy + eps * eps);
                #pragma omp atomic
                potentialEnergy -= (G * m * particles[j].getMass()) / r;
            }
        }
    }
    double totalEnergy = kineticEnergy + potentialEnergy;

    energyFile << std::fixed << std::setprecision(8) 
            << kineticEnergy << " \t " 
            << potentialEnergy << " \t " 
            << totalEnergy << "\n";
}


void NBodySimulator::processBodies(std::ostream &energyFile) {
    system->computeAccelerations(); //obtengo las aceleraciones
    integrateEuler(); //muevo las particulas
    calculateEnergy(energyFile); //calculo la energía
}


//Version con parallel for
//Importante si se quiere comparar con el task usar el method = 1
void NBodySimulator::processBodies(std::ostream &energyFile, int method, int syncType ,int scheduleType, int chunkSize, bool use_barrier) {
    system->computeAccelerations(scheduleType, chunkSize);
    if(syncType == 2) integrateEuler(syncType,use_barrier); //muevo las particulas
    else integrateEuler(syncType);
    calculateEnergy(energyFile, method); //calculo la energía
}


//utiliza metodo atomico para calcular la energía y el método de tareas para calcular las aceleraciones
void NBodySimulator::processBodies(std::ostream &energyFile, int taskType,int syncType) {
    system->computeAccelerations(taskType);
    integrateEuler(syncType); //muevo las particulas
    calculateEnergy(energyFile, 1); //calculo la energía
}


void NBodySimulator::simulate(int steps, std::string energyFilename, std::string trajectoryFilename, int sim_type, int syncType, int scheduleType, int chunkSize, int method, int kernelVariant, int taskType, bool use_barrier) {
    //creacion del archivo de energias y trayectorias .dat
    std::ofstream energyFile;
    std::ofstream trajectoryFile;

    energyFile.open(energyFilename, std::ios::out);
    if (!energyFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo de energia: " << energyFilename << std::endl;
        return;
    }

    if (energyFile.tellp() == 0) {
        energyFile << "K_Cinetica \t U_Potencial \t E_Total\n";
    }

    trajectoryFile.open(trajectoryFilename, std::ios::out);
    if (!trajectoryFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo de trayectorias: " << trajectoryFilename << std::endl;
        return;
    }

    if (trajectoryFile.tellp() == 0) {
        trajectoryFile << "id \t step \t X \t Y \t VX \t VY\n";
    }

    auto start = std::chrono::high_resolution_clock::now();


    // Simulacion basada en parametros de entrada, se pueden elegir entre 4 tipos de simulacion: 
    // 0 = serial, 1 = paralelo con for, 2 = paralelo con tareas, 3 = CUDA
    if(sim_type == 0) {
        for(int step = 0; step < steps; ++step){
            this->processBodies(energyFile);
            system->saveSnapshot(trajectoryFile, step); 
            if (step % 10 == 0) { 
                std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
            }
            std::cout.flush();
        }
    }
    
    else if(sim_type == 3) {
        int N = system->getCount();
        //double G = system->getG_const();
        //double eps = system->getEps();
        int blockSize = chunkSize; 
        
        CudaBuffer buffer(N, system->getParticles());

        if (method == 0) { //memoria compartida con shared
            if (kernelVariant == 0) {
                for (int step = 0; step < steps; ++step){
                    system->computeAccelerationsGpu(0, blockSize); // Usamos la variante básica para este método
                    this->stepEulerGpu(&buffer);
                    this->calculateEnergyGpu(method, &buffer, energyFile);
                    
                    system->saveSnapshot(trajectoryFile, step); 
                    
                    if (step % 10 == 0) { 
                        std::cout << "ciclo " << step + 1  << " listo (CUDA)" << std::endl; 
                    }
                    std::cout.flush();
                }
            } else if (kernelVariant == 1) {
                for (int step = 0; step < steps; ++step){
                    system->computeAccelerationsGpu(1, blockSize); // Usamos la variante con shared memory para este método
                    
                    this->stepEulerGpu(&buffer);
                    this->calculateEnergyGpu(method, &buffer, energyFile);
                    
                    system->saveSnapshot(trajectoryFile, step); 
                    
                    if (step % 10 == 0) { 
                        std::cout << "ciclo " << step + 1  << " listo (CUDA)" << std::endl; 
                    }
                    std::cout.flush();
                }
            } else {
                std::cerr << "Variante de kernel desconocida: " << kernelVariant << std::endl;
                return;
            }
        } 
        else if(method == 1) { //atomic add
            if (kernelVariant == 0) {
                for (int step = 0; step < steps; ++step){
                    system->computeAccelerationsGpu(0, blockSize); // Usamos la variante básica para este método
                    this->stepEulerGpu(&buffer);
                    this->calculateEnergyGpu(method, &buffer, energyFile);
                    
                    system->saveSnapshot(trajectoryFile, step); 
                    
                    if (step % 10 == 0) { 
                        std::cout << "ciclo " << step + 1  << " listo (CUDA)" << std::endl; 
                    }
                    std::cout.flush();
                }
            } else if (kernelVariant == 1) {
                for (int step = 0; step < steps; ++step){
                    system->computeAccelerationsGpu(1, blockSize); // Usamos la variante con shared memory para este método
                    
                    this->stepEulerGpu(&buffer);
                    this->calculateEnergyGpu(method, &buffer, energyFile);
                    
                    system->saveSnapshot(trajectoryFile, step); 
                    
                    if (step % 10 == 0) { 
                        std::cout << "ciclo " << step + 1  << " listo (CUDA)" << std::endl; 
                    }
                    std::cout.flush();
                }
            } else {
                std::cerr << "Variante de kernel desconocida: " << kernelVariant << std::endl;
                return;
            }
        }
        else {
            std::cerr << "Metodo de paralelizacion no valido. Use 0 para Reduccion compartida o 1 para AtomicAdd." << std::endl;
            return;
        }
    }

    else {
        if(taskType == -1){
            for (int step = 0; step < steps; ++step){
                this->processBodies(energyFile, method, syncType, scheduleType, chunkSize, use_barrier);
                system->saveSnapshot(trajectoryFile, step); 
                if (step % 10 == 0) { 
                    std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
                }
                std::cout.flush();
            }
        } else {
            for (int step = 0; step < steps; ++step){
                this->processBodies(energyFile, taskType, syncType);
                system->saveSnapshot(trajectoryFile, step); 
                if (step % 10 == 0) { 
                    std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
                }
                std::cout.flush();
            }
        }
    }

    energyFile.close();
    trajectoryFile.close();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    std::cout << "Simulacion hecha en " 
              << std::fixed << std::setprecision(8)
              << duration.count() << " segundos." << std::endl;
    return;
}