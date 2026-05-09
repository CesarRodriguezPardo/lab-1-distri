# include "NBodySimulator.h"

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


void NBodySimulator::simulate(int steps, std::string energyFilename, std::string trajectoryFilename, int sim_type, int syncType, int scheduleType, int chunkSize, int method, int taskType, bool use_barrier) {
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


    // Simulacion basada en parametros de entrada, se pueden elegir entre 3 tipos de simulacion: 
    // 0 = serial, 1 = paralelo con for, 2 = paralelo con tareas
    if(sim_type == 0){
        for(int step = 0; step < steps; ++step){
            this->processBodies(energyFile);
            system->saveSnapshot(trajectoryFile, step); // Guardar el estado actual en el archivo
            if (step % 10 == 0) { // Imprimir cada 10 pasos para no saturar la salida
                std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
            }
            std::cout.flush();
        }
    }else {
        if(taskType == -1){
            for (int step = 0; step < steps; ++step){
                this->processBodies(energyFile, method, syncType, scheduleType, chunkSize, use_barrier);
                system->saveSnapshot(trajectoryFile, step); // Guardar el estado actual en el archivo
                if (step % 10 == 0) { // Imprimir cada 10 pasos para no saturar la salida
                    std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
                }
                std::cout.flush();
            }

        } else{
            for (int step = 0; step < steps; ++step){
                this->processBodies(energyFile, taskType, syncType);
                system->saveSnapshot(trajectoryFile, step); // Guardar el estado actual en el archivo
                if (step % 10 == 0) { // Imprimir cada 10 pasos para no saturar la salida
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

    std::cout << "Simulation done in " 
              << std::fixed << std::setprecision(8)
              << duration.count() << " seconds." << std::endl;
    return;
}