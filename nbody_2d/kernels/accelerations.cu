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
}