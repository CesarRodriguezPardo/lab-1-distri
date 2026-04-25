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

void NBodySimulator::calculateEnergy(std::ofstream &energyFile){
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

void NBodySimulator::processBodies(std::ofstream &energyFile) {
    system->computeAccelerations(); //obtengo las aceleraciones

    integrateEuler(); //muevo las particulas

    calculateEnergy(energyFile);
}

void NBodySimulator::simulate(int steps) {
    //creacion del archivo de energias.dat
    static bool exists = true;
    std::ofstream energyFile;

    if (exists) {
        // La primera vez abrimos con ios::out (por defecto borra lo anterior si existe)
        energyFile.open("energy_timeseries.dat", std::ios::out);
        exists = false; 
    } else {
        // Las siguientes veces del bucle abrimos con ios::app (añadir al final)
        energyFile.open("energy_timeseries.dat", std::ios::app);
    }
    energyFile.seekp(0, std::ios::end); 
    if (energyFile.tellp() == 0) {
        energyFile << "K_Cinetica \t U_Potencial \t E_Total\n";
    }

    //creacion del archivo de trayectorias
    std::ofstream file("trajectories.dat");
    file << "id \t step \t X \t Y \t VX \t VY\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int step = 0; step < steps; ++step){
        this->processBodies(energyFile);
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