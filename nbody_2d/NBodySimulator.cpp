# include "NBodySimulator.h"

// Source - https://stackoverflow.com/a/18773495
// Posted by IInspectable, modified by community. See post 'Timeline' for change history
// Retrieved 2026-04-09, License - CC BY-SA 3.0
const double EulerConstant = std::exp(1.0);
 
NBodySimulator::NBodySimulator(NBodySystem* sys, double dt)
    : system(sys), 
      time_step(dt < EulerConstant ? EulerConstant : dt) {}

void NBodySimulator::integrateEuler(){
    auto& particles = system->getParticles();
    int n = particles.size();

    for (int i = 0; i < n; ++i){
    // 1. Obtener datos actuales
        double vx = particles[i].getVX();
        double vy = particles[i].getVY();
        double ax = particles[i].getAX();
        double ay = particles[i].getAY();

        // 2. Calcular nueva velocidad (v = v + a * dt)
        double next_vx = vx + ax * time_step;
        double next_vy = vy + ay * time_step;

        // 3. Calcular nueva posición (p = p + v_nueva * dt)
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

void NBodySimulator::simulate(int N, double simG, double simEps, double delta_time) {
    return;

}