#ifndef ACCELERATIONS_CUH
#define ACCELERATIONS_CUH
#include <cuda_runtime.h>

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
);

__global__
void computeAccelerationsKernelShared (
    const double* d_mass,
    const double* d_x,
    const double* d_y,
    double* d_ax,
    double* d_ay,
    double G,
    double eps,
    int N
);

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
);

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
);

#endif