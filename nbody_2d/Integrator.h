#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "NBodySystem.h"
#include <omp.h>

/**
 * @class Integrator
 * @brief Responsable de integrar las ecuaciones de movimiento (Euler).
 *
 * Desacopla la lógica de integración de NBodySimulator.
 * Implementa tres variantes del método de Euler:
 *   - Serial (sin OpenMP)
 *   - Paralelo con sincronización configurable (critical / nowait)
 *   - Paralelo con control explícito de barrier
 */
class Integrator {
private:
    NBodySystem* system;
    double       time_step;

public:
    Integrator(NBodySystem* sys, double dt);

    // ── Variante 1: integración serial ──────────────────────────
    void integrateEuler();

    // ── Variante 2: paralelo con tipo de sincronización ──────────
    //   syncType = 1 → #pragma omp critical
    //   syncType = 2 → #pragma omp for nowait
    void integrateEuler(int syncType);

    // ── Variante 3: paralelo nowait con barrier opcional ─────────
    //   use_barrier = true  → #pragma omp barrier explícito
    //   use_barrier = false → sin barrera (nowait puro)
    void integrateEuler(int syncType, bool use_barrier);
};

#endif
