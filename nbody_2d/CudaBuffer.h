
#ifndef CUDABUFFER_H
#define CUDABUFFER_H

#include <iostream>
#include <vector>
#include <cuda_runtime.h>

#include "Particle.h"

class CudaBuffer{
    public:
        double *d_mass; //masas
        double *d_x;    //posicion x
        double *d_y;    //posicion y
        double *d_ax;   //aceleracion x
        double *d_ay;   //aceleracion y

        // Constructores
        CudaBuffer(int n); // Alojamiento de memoria 
        CudaBuffer(int n, std::vector<Particle> bodies);

        ~CudaBuffer();

};

#endif