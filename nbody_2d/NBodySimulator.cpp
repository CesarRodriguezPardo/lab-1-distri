# include "NBodySimulator.h"
 
NBodySimulator::NBodySimulator(NBodySystem* sys, double dt)
    : system(sys), time_step(dt){}
      

void NBodySimulator::integrateEuler(){
    auto& particles = system->getParticles();
    int n = particles.size();

    for (int i = 0; i < n; ++i){
        particles[i].kick(time_step);
        particles[i].drift(time_step);
    }
}

void NBodySimulator::integrateEuler(int syncType) {
    auto& particles = system->getParticles();
    int n = particles.size();

    switch (syncType)
    {    
    case 1: // critical
        for (int i = 0; i < n; ++i){
            #pragma omp critical
            {
                particles[i].kick(time_step);
                particles[i].drift(time_step);
            }
        }
        break;
    case 2: // nowait
        #pragma omp parallel
        #pragma omp for nowait
         for (int i = 0; i < n; ++i){
            particles[i].kick(time_step);
            particles[i].drift(time_step);
        }

        break;
    
    default:
        break;
    }
    
}

//Ajustar para utilizar un collapse(2), ver posibilidad de utilizar critics o tasks
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

            #pragma omp atomic
            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
        for (int j = i + 1; j < n; ++j) {
            double dx = particles[j].getX() - xi;
            double dy = particles[j].getY() - yi;
            
            // Usamos el mismo suavizado 'eps' que en las aceleraciones
            
            double r = sqrt(dx * dx + dy * dy + eps * eps);
                #pragma omp atomic
                potentialEnergy -= (G * m * particles[j].getMass()) / r;
        }
    }
    double totalEnergy = kineticEnergy + potentialEnergy;

    energyFile << std::fixed << std::setprecision(8) 
            << kineticEnergy << " \t " 
            << potentialEnergy << " \t " 
            << totalEnergy << "\n";
}

void NBodySimulator::calculateEnergy(std::ostream &energyFile, int sim_type) {
    auto& particles = system->getParticles();
    int n = particles.size();
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    double G = system->getG_const();
    double eps = system->getEps();

    if (sim_type == 1) {
        // Parallel for version
        #pragma omp parallel for reduction(+:kineticEnergy, potentialEnergy) schedule(static)
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
    } else if (sim_type == 2) {
        // Task-based version

        #pragma omp parallel
        {
            #pragma omp single
            {
                for (int i = 0; i < n; ++i){
                    #pragma omp task firstprivate(i) shared(kineticEnergy, potentialEnergy)
                    {
                        double m = particles[i].getMass();
                        double vx = particles[i].getVX();
                        double vy = particles[i].getVY();
                        double xi = particles[i].getX();
                        double yi = particles[i].getY();

                        {
                            #pragma omp atomic
                            kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
                        }
                        for (int j = i + 1; j < n; ++j) {
                            double dx = particles[j].getX() - xi;
                            double dy = particles[j].getY() - yi;
                            
                            double r = sqrt(dx * dx + dy * dy + eps * eps);
                            {
                                #pragma omp atomic
                                potentialEnergy -= (G * m * particles[j].getMass()) / r;
                            }
                        }
                    }
                }
            }
        }
        double totalEnergy = kineticEnergy + potentialEnergy;

        energyFile << std::fixed << std::setprecision(8) 
                << kineticEnergy << " \t " 
                << potentialEnergy << " \t " 
                << totalEnergy << "\n";
    }
}

void NBodySimulator::processBodies(std::ostream &energyFile, int sim_type, int syncType ,int scheduleType, int chunkSize) {
    if (sim_type == 0) {
        system->computeAccelerations(); //obtengo las aceleraciones
        integrateEuler(); //muevo las particulas
        calculateEnergy(energyFile); //calculo la energía
    } else if (sim_type == 1) {
        system->computeAccelerations(scheduleType, chunkSize);
        integrateEuler(syncType); //muevo las particulas
        calculateEnergy(energyFile, sim_type); //calculo la energía
    } else if (sim_type == 2) {
        system->computeAccelerationsTasks();
        integrateEuler(syncType); //muevo las particulas
        calculateEnergy(energyFile, sim_type); //calculo la energía
    }
}

void NBodySimulator::simulate(int steps, std::string energyFilename, std::string trajectoryFilename, int sim_type, int syncType, int scheduleType, int chunkSize) {
    //creacion del archivo de energias.dat
    std::ofstream energyFile;

    energyFile.open(energyFilename, std::ios::out);
    if (!energyFile.is_open()) {
        std::cerr << "No se pudo abrir el archivo de energia: " << energyFilename << std::endl;
        return;
    }

    if (energyFile.tellp() == 0) {
        energyFile << "K_Cinetica \t U_Potencial \t E_Total\n";
    }

    //creacion del archivo de trayectorias
    std::ofstream file(trajectoryFilename, std::ios::out);
    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo de trayectorias: " << trajectoryFilename << std::endl;
        return;
    }
    file << "id \t step \t X \t Y \t VX \t VY\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int step = 0; step < steps; ++step){
        this->processBodies(energyFile, sim_type, syncType, scheduleType, chunkSize);
        system->saveSnapshot(file, step); // Guardar el estado actual en el archivo
        std::cout << "ciclo " << step + 1  << " listo" << std::endl; 
    }

    energyFile.close();

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    std::cout << "Simulation done in " 
              << std::fixed << std::setprecision(8)
              << duration.count() << " seconds." << std::endl;
    return;
}