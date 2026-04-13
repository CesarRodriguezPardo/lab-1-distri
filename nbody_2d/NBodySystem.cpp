#include "NBodySystem.h"
#include <cmath>
#include <iostream>
#include <random>
#include <fstream>

NBodySystem::NBodySystem(double G, double epsilon) 
    : G_const(G), eps(epsilon) {}

void NBodySystem::addParticle(const Particle& p){
    bodies.push_back(p);
    //bodies.emplace_back(p); esto se suponque que podría ahorrarle tiempo al procesador (faltan test)
}

/*
void NBodySystem::zeroAccelerations (){
    for(auto& body : bodies) {
        body.setAcceleration(0.0 , 0.0);
    }
}
*/

void NBodySystem::computeAccelerations(){
    int nBodies = bodies.size();
    //double distanceX, distanceY, rSquared, r; - si se declaran aquí
    //para el serial es mejor porque no tiene que declarar en cada bucle
    //pero para el paralelizado se forma una condicion de carrera, entonces
    //es mejor declararlo dentro del ciclo, para que cada hilo tenga su variable propia

    //Doble bucle para interacciones pareja-a-pareja
    for (int i = 0; i < nBodies; ++i) {
        double distanceX, distanceY, rSquared, r;
        double totalAX = 0;
        double totalAY = 0;
        for (int j = 0; j < nBodies; ++j) {

            if (i != j) {
                distanceX = bodies[j].getX() - bodies[i].getX();
                distanceY = bodies[j].getY() - bodies[i].getY();
                rSquared = distanceX * distanceX
                         + distanceY * distanceY
                         + eps * eps;
                r = sqrt(rSquared);
                totalAX += (G_const * bodies[j].getMass() * distanceX) / (r * r * r);
                totalAY += (G_const * bodies[j].getMass() * distanceY) / (r * r * r);
            }
        }
        bodies[i].setAcceleration(totalAX, totalAY);
    }
}


const std::vector <Particle>& NBodySystem::getBodies() const {
    return bodies;
}

int NBodySystem::getCount() const {
    return bodies.size();
}

void NBodySystem::randomSystem (int amount, int seed){
    srand(seed);
    for (int i = 0; i < amount; ++i){
        double randomX = (double)rand() / RAND_MAX * 100.0;
        double randomY = (double)rand() / RAND_MAX * 100.0;
        double randomMass = 1.0 + (double)rand() / RAND_MAX * 10.0;

        //se podrían guardar velocidades aleatorias en caso de querer hacer otras estructuras;
        //double vx = -1.0 + (double)rand() / RAND_MAX * 2.0; 
        //double vy = -1.0 + (double)rand() / RAND_MAX * 2.0;


        Particle p(randomMass, randomX, randomY);
        
        // 4. Agregar al vector 'bodies' (usa push_back)
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