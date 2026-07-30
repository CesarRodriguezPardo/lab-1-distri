#ifndef METRICS_CUH
#define METRICS_CUH
#include "../CudaBuffer.h"

// Variante 1: Reducción usando atomicAdd
void launchKineticAtomic(CudaBuffer* buf, double* d_K, int N, int blockSize);
void launchPotentialAtomic(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize);

// Variante 0: Reducción en memoria compartida (__shared__)
void launchKineticShared(CudaBuffer* buf, double* d_K, int N, int blockSize);
void launchPotentialShared(CudaBuffer* buf, double* d_U, int N, double G, double eps, int blockSize);

#endif // METRICS_CUH