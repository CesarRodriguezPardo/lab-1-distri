#include <cuda_runtime.h>
#include <cmath>
#include "metrics.cuh"

// =========================================================================
// VARIANTE 1: atomicAdd (Método 1)
// =========================================================================

__global__ void kineticEnergyAtomicKernel(double* d_mass, double* d_vx, double* d_vy, double* d_total_K, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) {
        double v2 = (d_vx[i] * d_vx[i]) + (d_vy[i] * d_vy[i]);
        double local_K = 0.5 * d_mass[i] * v2;
        atomicAdd(d_total_K, local_K);
    }
}

__global__ void potentialEnergyAtomicKernel(double* d_mass, double* d_x, double* d_y, double* d_total_U, int N, double G, double eps) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N) {
        double local_U = 0.0;
        for (int j = i + 1; j < N; ++j) {
            double dx = d_x[j] - d_x[i];
            double dy = d_y[j] - d_y[i];
            double dist2 = (dx * dx) + (dy * dy);
            local_U -= (G * d_mass[i] * d_mass[j]) / sqrt(dist2 + (eps * eps));
        }
        atomicAdd(d_total_U, local_U);
    }
}

// =========================================================================
// VARIANTE 0: Reducción en Memoria Compartida (Método 0)
// =========================================================================

__global__ void kineticEnergySharedKernel(double* d_mass, double* d_vx, double* d_vy, double* d_total_K, int N) {
    // Memoria compartida dinámica para el bloque
    extern __shared__ double sdata[]; 
    
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // 1. Cada hilo calcula su K y la guarda en la memoria compartida
    double my_K = 0.0;
    if (i < N) {
        double v2 = (d_vx[i] * d_vx[i]) + (d_vy[i] * d_vy[i]);
        my_K = 0.5 * d_mass[i] * v2;
    }
    sdata[tid] = my_K;
    __syncthreads(); // Esperamos a que todo el bloque guarde sus datos

    // 2. Reducción en árbol (stride se divide a la mitad en cada paso)
    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    // 3. El hilo líder del bloque suma el subtotal a la variable global
    if (tid == 0) {
        atomicAdd(d_total_K, sdata[0]);
    }
}

__global__ void potentialEnergySharedKernel(double* d_mass, double* d_x, double* d_y, double* d_total_U, int N, double G, double eps) {
    extern __shared__ double sdata[];
    
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    double my_U = 0.0;
    if (i < N) {
        for (int j = i + 1; j < N; ++j) {
            double dx = d_x[j] - d_x[i];
            double dy = d_y[j] - d_y[i];
            double dist2 = (dx * dx) + (dy * dy);
            my_U -= (G * d_mass[i] * d_mass[j]) / sqrt(dist2 + (eps * eps));
        }
    }
    sdata[tid] = my_U;
    __syncthreads();

    for (unsigned int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] += sdata[tid + s];
        }
        __syncthreads();
    }

    if (tid == 0) {
        atomicAdd(d_total_U, sdata[0]);
    }
}

void launchKineticAtomic(CudaBuffer* buf, double* d_K, int N, int blockSize) {
    int numBlocks = (N + blockSize - 1) / blockSize;
    kineticEnergyAtomicKernel<<<numBlocks, blockSize>>>(buf->d_mass, buf->d_vx, buf->d_vy, d_K, N);
}

void launchPotentialAtomic(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize) {
    int numBlocks = (N + blockSize - 1) / blockSize;
    potentialEnergyAtomicKernel<<<numBlocks, blockSize>>>(buf->d_mass, buf->d_x, buf->d_y, d_U, N, G, eps);
}

void launchKineticShared(CudaBuffer* buf, double* d_K, int N, int blockSize) {
    int numBlocks = (N + blockSize - 1) / blockSize;
    size_t sharedBytes = blockSize * sizeof(double);
    kineticEnergySharedKernel<<<numBlocks, blockSize, sharedBytes>>>(buf->d_mass, buf->d_vx, buf->d_vy, d_K, N);
}

void launchPotentialShared(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize) {
    int numBlocks = (N + blockSize - 1) / blockSize;
    size_t sharedBytes = blockSize * sizeof(double); // Memoria dinámica
    potentialEnergySharedKernel<<<numBlocks, blockSize, sharedBytes>>>(buf->d_mass, buf->d_x, buf->d_y, d_U, N, G, eps);
}