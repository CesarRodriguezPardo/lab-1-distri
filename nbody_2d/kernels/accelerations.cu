#include "CudaBuffer.h"
#include "accelerations.cuh"
#include <cmath>

__global__
void computeAccelerationsKernel(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    double G,
    double eps,
    int N
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= N) return;  // Protección de bordes

    double totalAX = 0.0;
    double totalAY = 0.0;
    double xi = d_x[i];
    double yi = d_y[i];
    for (int j = 0; j < N; ++j) {
        if (i != j) {
            double dx = d_x[j] - xi;
            double dy = d_y[j] - yi;
            double rSquared = dx * dx + dy * dy + eps * eps;
            double r = sqrt(rSquared);
            double scalarForce = (G * d_mass[j]) / (rSquared * r);
            totalAX += scalarForce * dx;
            totalAY += scalarForce * dy;
        }
    }
    d_ax[i] = totalAX;
    d_ay[i] = totalAY;
    
}

extern __shared__ double shared[];
__global__
void computeAccelerationsKernelShared(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    double G,
    double eps,
    int N
)
{
    double* sharedMass = shared;
    double* sharedX    = shared + blockDim.x;
    double* sharedY    = shared + 2 * blockDim.x;

    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i >= N) return;    // Protección de bordes

    double xi = d_x[i];
    double yi = d_y[i];

    double totalAX = 0.0;
    double totalAY = 0.0;

    // Recorrer partículas por bloques (tiles)
    for (int tile = 0; tile < N; tile += blockDim.x) {
        int globalIdx = tile + threadIdx.x;

        // Cargar tile a shared memory
        if (globalIdx < N) {
            sharedMass[threadIdx.x] = d_mass[globalIdx];
            sharedX[threadIdx.x]    = d_x[globalIdx];
            sharedY[threadIdx.x]    = d_y[globalIdx];
        } else {
            sharedMass[threadIdx.x] = 0.0;
            sharedX[threadIdx.x]    = 0.0;
            sharedY[threadIdx.x]    = 0.0;
        }

        __syncthreads();

        // Calcular tamaño del tile (puede ser menor que blockDim.x en el último tile)
        int tileSize = min(blockDim.x, N - tile);

        for (int j = 0; j < tileSize; j++) {
            int globalJ = tile + j;

            if (globalJ == i) continue;

            double dx = sharedX[j] - xi;
            double dy = sharedY[j] - yi;

            double rSquared = dx * dx + dy * dy + eps * eps;
            double r = sqrt(rSquared);
            double scalarForce = G * sharedMass[j] / (rSquared * r);

            totalAX += scalarForce * dx;
            totalAY += scalarForce * dy;
        }

        // Esperar antes de sobrescribir shared memory
        __syncthreads();
    }

    d_ax[i] = totalAX;
    d_ay[i] = totalAY;
}

void launchComputeAccelerationsKernel(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    double G,
    double eps,
    int N,
    int blockSize
) {
    int gridSize = (N + blockSize - 1) / blockSize;
    computeAccelerationsKernel<<<gridSize, blockSize>>>(
        d_mass, d_x, d_y, d_ax, d_ay, G, eps, N
    );
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaGetLastError());
}

void launchComputeAccelerationsKernelShared(
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    double G,
    double eps,
    int N,
    int blockSize
) {
    int gridSize = (N + blockSize - 1) / blockSize;
    size_t sharedMemSize = 3 * blockSize * sizeof(double); // mass, x, y
    computeAccelerationsKernelShared<<<gridSize, blockSize, sharedMemSize>>>(
        d_mass, d_x, d_y, d_ax, d_ay, G, eps, N
    );
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaGetLastError());
}