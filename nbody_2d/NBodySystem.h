#ifndef NBODYSYSTEM_H
#define NBODYSYSTEM_H

#include <vector>
#include <cstdlib>
#include "Particle.h"
#include <omp.h>

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
        void computeAccelerations(int scheduleType, int chunkSize);
        void computeAccelerations(int taskType);
        const std::vector<Particle>& getBodies() const;
        int getCount() const;
        void randomSystem(int amount, int seed = 1);
        void bynarySystem(int seed);
        void diskSystem(int amount, int seed);
        void saveSnapshot(std::ostream& outFile, int step);

        //getters
        std::vector<Particle>& getParticles() { return bodies;}
        double getG_const() const {return G_const;}
        double getEps() const {return eps;}
};

# endif