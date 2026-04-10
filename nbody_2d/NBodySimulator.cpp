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

        // usar kick
        double next_vx = vx + ax * time_step;
        double next_vy = vy + ay * time_step;

        // usar drift
        double next_x = particles[i].getX() + next_vx * time_step;
        double next_y = particles[i].getY() + next_vy * time_step;

        // 4. Guardar cambios en la partícula
        particles[i].setVelocity(next_vx, next_vy);
        particles[i].setPosition(next_x, next_y);
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

    std::ofstream outFile("evolucion_energia.dat", std::ios::app);
    if (outFile.is_open()){
        outFile << std::fixed << std::setprecision(8) << totalEnergy << std::endl;

        outFile.close();
    }
}

void NBodySimulator::processBodies() {

    system->computeAccelerations(); //obtengo las aceleraciones

    integrateEuler(); //muevo las particulas

    calculateEnergy(); //opcional pero sirve
}

void NBodySimulator::simulate(int steps) {
    for (int i = 0; i < steps; i++){
        this->processBodies();
        std::cout << "ciclo " << i+1  << " listo" << std::endl; 
    }
    return;

}