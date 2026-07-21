
#ifndef CUDABUFFER_H
#define CUDABUFFER_H

#include <iostream>
#include <vector>
#include <cuda_runtime.h>
#include <cstdlib> // Para std::exit

#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "Error CUDA en " << __FILE__ << ":" << __LINE__ \
                      << " - Código: " << err << " (" \
                      << cudaGetErrorString(err) << ")" << std::endl; \
            std::exit(EXIT_FAILURE); \
        } \
    } while (0)

#include "Particle.h"

class CudaBuffer{
    public:
        double *d_mass; //masas

        double *d_x;    //posicion x
        double *d_y;    //posicion y

        double *d_ax;   //aceleracion x
        double *d_ay;   //aceleracion y

        double *d_vx;   //velocidad x
        double *d_vy;   //velocidad y

        // Constructores
        CudaBuffer(int n){ // Constructor que reserva memoria en el device para n particulas
            CUDA_CHECK(cudaMalloc((void**)&d_mass, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_x, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_y, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_ax, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_ay, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_vx, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_vy, n * sizeof(double)));
        }

        // Constructor que realiza preprocesamiento del vector de particulas y realiza transferencia a device
        CudaBuffer(int n, std::vector<Particle>& bodies){ 

            std::vector<double> h_mass(n);

            std::vector<double> h_x(n);
            std::vector<double> h_y(n);

            std::vector<double> h_vx(n);
            std::vector<double> h_vy(n);

            for (int i = 0; i < n; i++){
                h_mass[i] = bodies[i].getMass();

                h_x[i] = bodies[i].getX();
                h_y[i] = bodies[i].getY();

                h_vx[i] = bodies[i].getVX();
                h_vy[i] = bodies[i].getVY();
            }

            CUDA_CHECK(cudaMalloc((void**)&d_mass, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_x, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_y, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_ax, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_ay, n * sizeof(double)));

            CUDA_CHECK(cudaMalloc((void**)&d_vx, n * sizeof(double)));
            CUDA_CHECK(cudaMalloc((void**)&d_vy, n * sizeof(double)));

            //Transferencia

            CUDA_CHECK(cudaMemcpy(d_mass, h_mass.data(), n * sizeof(double), cudaMemcpyHostToDevice));
            
            CUDA_CHECK(cudaMemcpy(d_x, h_x.data(), n * sizeof(double), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_y, h_y.data(), n * sizeof(double), cudaMemcpyHostToDevice));

            CUDA_CHECK(cudaMemcpy(d_vx, h_vx.data(), n * sizeof(double), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_vy, h_vy.data(), n * sizeof(double), cudaMemcpyHostToDevice));

        }

        // Destructor
        ~CudaBuffer(){
            CUDA_CHECK(cudaFree(d_mass));

            CUDA_CHECK(cudaFree(d_x));
            CUDA_CHECK(cudaFree(d_y));

            CUDA_CHECK(cudaFree(d_ax));
            CUDA_CHECK(cudaFree(d_ay));

            CUDA_CHECK(cudaFree(d_vx));
            CUDA_CHECK(cudaFree(d_vy));
        };

        void retrieveAccelerations(std::vector<Particle>& bodies){
            CUDA_CHECK(cudaDeviceSynchronize());

            int n = bodies.size();
            
            std::vector<double> h_ax(n);
            std::vector<double> h_ay(n);


            CUDA_CHECK(cudaMemcpy(h_ax.data(), d_ax, n * sizeof(double), cudaMemcpyDeviceToHost));
            CUDA_CHECK(cudaMemcpy(h_ay.data(), d_ay, n * sizeof(double), cudaMemcpyDeviceToHost));

            for(int i = 0; i < n; i++){
                bodies[i].setAcceleration(h_ax[i], h_ay[i]);
            }

        }

        void updateDeviceKinematics(std::vector<Particle>& bodies){
            int n = bodies.size();

            std::vector<double> h_x(n);
            std::vector<double> h_y(n);

            std::vector<double> h_vx(n);
            std::vector<double> h_vy(n);

            for (int i = 0; i < n; i++){

                h_x[i] = bodies[i].getX();
                h_y[i] = bodies[i].getY();

                h_vx[i] = bodies[i].getVX();
                h_vy[i] = bodies[i].getVY();
            }

            CUDA_CHECK(cudaMemcpy(d_x, h_x.data(), n * sizeof(double), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_y, h_y.data(), n * sizeof(double), cudaMemcpyHostToDevice));

            CUDA_CHECK(cudaMemcpy(d_vx, h_vx.data(), n * sizeof(double), cudaMemcpyHostToDevice));
            CUDA_CHECK(cudaMemcpy(d_vy, h_vy.data(), n * sizeof(double), cudaMemcpyHostToDevice));
        }

};

#endif