#include "NBodySystem.h"
#include <cmath>
#include <iostream>


NBodySystem::NBodySystem(double G, double epsilon) 
    : G_const(G), eps(epsilon) {}

void NBodySystem::addParticle(const Particle& p){
    bodies.push_back(p);
}

void NBodySystem::zeroAccelerations (){
    for(auto& body : bodies) {
        body.setAcceleration(0.0 , 0.0);
    }
}

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

