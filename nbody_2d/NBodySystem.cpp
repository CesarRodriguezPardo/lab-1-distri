#include "NBodySystem.h"
#include <cmath>
#include <iostream>
#include <random>
#include <fstream>

NBodySystem::NBodySystem(double G, double epsilon) 
    : G_const(G), eps(epsilon) {}

void NBodySystem::addParticle(const Particle& p){
    bodies.push_back(p);
}

void NBodySystem::zeroAccelerations() {
    for (auto& body : bodies) {
        body.setAcceleration(0.0, 0.0);
    }
}

void NBodySystem::computeAccelerations(){
    int nBodies = bodies.size();

    //Doble bucle para interacciones pareja-a-pareja
    for (int i = 0; i < nBodies; ++i) {
        double distanceX, distanceY, rSquared, r;
        double totalAX = 0;
        double totalAY = 0;
        double xi = bodies[i].getX();
        double yi = bodies[i].getY();
        for (int j = 0; j < nBodies; ++j) {

            if (i != j) {
                distanceX = bodies[j].getX() - xi;
                distanceY = bodies[j].getY() - yi;
                rSquared = distanceX * distanceX
                         + distanceY * distanceY
                         + eps * eps;
                r = sqrt(rSquared);
                double ScalarForce = (G_const * bodies[j].getMass()) / (rSquared * r);
                totalAX += ScalarForce * distanceX;
                totalAY += ScalarForce * distanceY;
            }
        }
        bodies[i].setAcceleration(totalAX, totalAY);
    }
}

// Función para mapear entero a constante de OpenMP
omp_sched_t getScheduleFromInt(int type) {
    switch (type) {
        case 1: return omp_sched_static;
        case 2: return omp_sched_dynamic;
        case 3: return omp_sched_guided;
        case 4: return omp_sched_auto;
        default: return omp_sched_static; // Default seguro
    }
}

void NBodySystem::computeAccelerations(int scheduleType, int chunkSize) {
    int nBodies = bodies.size();
    omp_set_schedule(getScheduleFromInt(scheduleType), chunkSize);

    #pragma omp for schedule(runtime)
    for (int i = 0; i < nBodies; ++i) {
        double totalAX = 0.0;
        double totalAY = 0.0;
        double xi = bodies[i].getX();
        double yi = bodies[i].getY();

        for (int j = 0; j < nBodies; ++j) {
            if (i == j) continue;
            double distanceX = bodies[j].getX() - xi;
            double distanceY = bodies[j].getY() - yi;
            double rSquared = distanceX * distanceX + distanceY * distanceY + eps * eps;
            double r = sqrt(rSquared);
            double ScalarForce = (G_const * bodies[j].getMass()) / (rSquared * r);
            totalAX += ScalarForce * distanceX;
            totalAY += ScalarForce * distanceY;
        }
        bodies[i].setAcceleration(totalAX, totalAY);
    }
}

void NBodySystem::computeAccelerations(int taskType) {
    int nBodies = bodies.size();
    if (taskType == 0) {
        // Tareas sin dependencias explícitas
        #pragma omp parallel
        {
            #pragma omp single // Solo un hilo crea las tareas
            {
                for (int i = 0; i < nBodies; ++i) {
                    #pragma omp task firstprivate(i) // Se dividen las tareas por cada partícula y se comparte el indice par utilizarlo dentro del for
                    {
                        double totalAX = 0.0;
                        double totalAY = 0.0;
                        double xi = bodies[i].getX();
                        double yi = bodies[i].getY();

                        for (int j = 0; j < nBodies; ++j) {
                            if (i == j) continue;
                            double distanceX = bodies[j].getX() - xi;
                            double distanceY = bodies[j].getY() - yi;
                            double rSquared = distanceX * distanceX + distanceY * distanceY + eps * eps;
                            double r = sqrt(rSquared);
                            double ScalarForce = (G_const * bodies[j].getMass()) / (rSquared * r);
                            totalAX += ScalarForce * distanceX;
                            totalAY += ScalarForce * distanceY;
                        }
                        bodies[i].setAcceleration(totalAX, totalAY);
                    }
                }
                #pragma omp taskwait // Espera a que todas las tareas terminen antes de salir de la región paralela
            }
        }
    }
}


const std::vector <Particle>& NBodySystem::getBodies() const {
    return bodies;
}

int NBodySystem::getCount() const {
    return bodies.size();
}

void NBodySystem::randomSystem(int amount, int seed) {
    std::mt19937 gen(seed);
    
    std::uniform_real_distribution<double> distPos(0.0, 100.0);
    std::uniform_real_distribution<double> distMass(1.0, 11.0); // 1.0 + max 10.0 = 11.0
    
    // Distribución para las velocidades, por si queremos comenzar con velocidades aleatorias
    // std::uniform_real_distribution<double> distVel(-1.0, 1.0);

    for (int i = 0; i < amount; ++i) {
        double randomX = distPos(gen);
        double randomY = distPos(gen);
        double randomMass = distMass(gen);

        // double vx = distVel(gen);
        // double vy = distVel(gen);

        Particle p(randomMass, randomX, randomY);
        
        this->addParticle(p);
    }
}

void NBodySystem::bynarySystem (int seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 100.0);

    // Centro aleatorio
    std::uniform_real_distribution<double> distCenter(25.0, 75.0);
    double centerX = distCenter(gen);
    double centerY = distCenter(gen);

    // Separación aleatoria
    std::uniform_real_distribution<double> distSep(20.0, 60.0);
    double separation = distSep(gen);

    // Masas grandes
    std::uniform_real_distribution<double> distMass(100.0, 200.0);
    double mass1 = distMass(gen);
    double mass2 = distMass(gen);

    // Masa ligera
    std::uniform_real_distribution<double> distLightMass(1.0, 10.0);
    double lightMass = distLightMass(gen);

    // Crear partículas
    Particle p1(mass1, centerX - separation / 2, centerY);
    Particle p2(mass2, centerX + separation / 2, centerY);

    // Posición masa ligera
    std::uniform_real_distribution<double> distPos(-10.0, 10.0);
    Particle p3(lightMass, centerX + distPos(gen), centerY + distPos(gen));

    // Agregar al sistema
    this->addParticle(p1);
    this->addParticle(p2);
    this->addParticle(p3);
}

void NBodySystem::diskSystem(int amount, int seed) {
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> distCanvas(0.0, 1.0);

    // Centro del disco
    double centerX = 50.0;
    double centerY = 50.0;

    // Radio aleatorio para las partículas
    std::uniform_real_distribution<double> distRadius(20.0, 45.0);
    double maxRadius = distRadius(gen);

    // Distribución de partículas
    for (int i = 0; i < amount; ++i) {
        double radius = std::sqrt(distCanvas(gen)) * maxRadius;
        double angle = distCanvas(gen) * 2.0 * M_PI;

        double x = centerX + radius * cos(angle);
        double y = centerY + radius * sin(angle);

        // Masa aleatoria
        std::uniform_real_distribution<double> distMass(1.0, 10.0);
        double mass = distMass(gen);

        Particle p(mass, x, y);
        this->addParticle(p);
    }
}

// Recibe una referencia al archivo ya abierto
void NBodySystem::saveSnapshot(std::ostream& outFile, int step) {
    for (size_t i = 0; i < bodies.size(); ++i) {
        const auto& p = bodies[i];
        outFile << i << " \t " << step << " \t " << p.getX() << " \t " << p.getY() << " \t "
                << p.getVX() << " \t " << p.getVY() << "\n";
    }
}