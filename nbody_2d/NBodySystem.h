#ifndef NBODYSYSTEM_H
#define NBODYSYSTEM_H

#include <vector>
#include "Particle.h"

using std::vector;

class NBodySystem {
    private:
        vector<Particle> bodies;
        double G_const;
        double eps;
    public:
        NBodySystem(double G, double epsilon);
        void addParticle(const Particle& p);
        void zeroAccelerations();
        void computeAccelerations();
        const std::vector<Particle>& getBodies() const;
        int getCount() const;
};

# endif