# include "NBodySimulator.h"
 
NBodySimulator::NBodySimulator(NBodySystem* sys, double dt)
    : system(sys), 
      time_step(dt)
      
      {/*energyFile.open("evolucion_energia.dat");*/}
      

void NBodySimulator::integrateEuler(){
    auto& particles = system->getParticles();
    int n = particles.size();

    for (int i = 0; i < n; ++i){
        double vx = particles[i].getVX();
        double vy = particles[i].getVY();
        double ax = particles[i].getAX();
        double ay = particles[i].getAY();


        particles[i].kick(time_step);
        particles[i].drift(time_step);
    }
}

void NBodySimulator::calculateEnergy(){
    auto& particles = system->getParticles();
    int n = particles.size();
    double kineticEnergy = 0.0;
    double potentialEnergy = 0.0;
    double G = system->getG_const();

    for (int i = 0; i < n; ++i){
        double m = particles[i].getMass();
        double vx = particles[i].getVX();
        double vy = particles[i].getVY();

        kineticEnergy += 0.5 * m * (vx * vx + vy * vy);
        for (int j = i + 1; j < n; ++j) {
            double dx = particles[j].getX() - particles[i].getX();
            double dy = particles[j].getY() - particles[i].getY();
            
            // Usamos el mismo suavizado 'eps' que en las aceleraciones
            double eps = system->getEps();
            double r = sqrt(dx * dx + dy * dy + eps * eps);
            
            potentialEnergy -= (G * m * particles[j].getMass()) / r;
        }
    }
    double totalEnergy = kineticEnergy + potentialEnergy;

    //creacion del archivo .dat
    static bool exists = true;
    std::ofstream outFile;

    if (exists) {
        // La primera vez abrimos con ios::out (por defecto borra lo anterior si existe)
        outFile.open("energy_timeseries.dat", std::ios::out);
        exists = false; 
    } else {
        // Las siguientes veces del bucle abrimos con ios::app (añadir al final)
        outFile.open("energy_timeseries.dat", std::ios::app);
    }
    
    if (outFile.is_open()) {
        //Revisamos si el archivo está vacío para poner el encabezado
        outFile.seekp(0, std::ios::end); 
        if (outFile.tellp() == 0) {
            outFile << "K_Cinetica \t U_Potencial \t E_Total\n";
        }
        outFile << std::fixed << std::setprecision(8) 
                << kineticEnergy << " \t " 
                << potentialEnergy << " \t " 
                << totalEnergy << "\n";
        
        outFile.close(); 
    }
}

void NBodySimulator::processBodies() {
    system->computeAccelerations(); //obtengo las aceleraciones

    integrateEuler(); //muevo las particulas

    calculateEnergy(); //opcional pero sirve
}

void NBodySimulator::simulate(int steps) {
    std::ofstream file("trajectories.dat");

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < steps; ++i){
        this->processBodies();
        system->saveSnapshot(file, i); // Guardar el estado actual en el archivo
        std::cout << "ciclo " << i + 1  << " listo" << std::endl; 
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> duration = end - start;

    std::cout << "Simulation done in " 
              << std::fixed << std::setprecision(8)
              << duration.count() << " seconds." << std::endl;
    return;
}