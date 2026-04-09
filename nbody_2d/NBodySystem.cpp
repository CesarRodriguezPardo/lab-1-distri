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
    //falta implementar

}

const std::vector <Particle>& NBodySystem::getBodies() const {
    return bodies;
}

int NBodySystem::getCount() const {
    return bodies.size();
}

