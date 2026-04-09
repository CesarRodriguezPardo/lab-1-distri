#pragma once

#include "NBodySystem.h"

/**
 * @brief Computes physical observables of the N-body system.
 *
 * Calculates kinetic energy (K), potential energy (U), total energy (E),
 * linear momentum, center of mass, and RMS radius.
 *
 * Week 1 : Stub — class declared, implementations pending.
 * Week 3 : Full serial and OpenMP-parallel implementations added, including:
 *   - calculateEnergy()
 *   - calculateEnergy(int method)          // 0=reduce, 1=atomic
 *   - calculateEnergy(int method, bool use_private)
 *   - calculateMetricsFirstprivate()
 *   - calculateFinalStateLastprivate()
 *
 * Physical formulas (adimensional units, G declared in NBodySystem):
 *
 *   K = (1/2) * sum_i  m_i * ||v_i||^2
 *
 *   U = -G * sum_{i<j}  m_i * m_j / sqrt(||r_j - r_i||^2 + eps^2)
 *
 *   E = K + U
 *
 *   P = sum_i  m_i * v_i          (linear momentum vector)
 *
 *   R_cm = (sum_i m_i * r_i) / M  (center of mass)
 *
 *   R_rms = sqrt( (1/M) * sum_i m_i * ||r_i - R_cm||^2 )
 *
 *   d_min = min_{i<j} ||r_j - r_i||   (closest pair — useful for encounter detection)
 */
class MetricsCalculator {
private:
    const NBodySystem* system;

    // Cached results from the last compute call
    double kineticEnergy;
    double potentialEnergy;
    double totalEnergy;
    double momentumX;
    double momentumY;
    double centerMassX;
    double centerMassY;
    double rmsRadius;
    double minDistance;

public:
    /**
     * @param sys Pointer to the N-body system (not owned — caller manages lifetime).
     */
    explicit MetricsCalculator(const NBodySystem* sys);

    // -----------------------------------------------------------------------
    // Week 3 — Serial reference implementations
    // -----------------------------------------------------------------------

    /** Computes K, U, E, momentum, center of mass, R_rms and d_min (serial). */
    void computeAll();

    /** Computes only kinetic energy K (serial). */
    double computeKineticEnergy() const;

    /** Computes only potential energy U (serial, O(N^2) pairs). */
    double computePotentialEnergy() const;

    // -----------------------------------------------------------------------
    // Week 3 — OpenMP parallel variants (declared here, implemented Week 3)
    // -----------------------------------------------------------------------

    /**
     * Computes K + U using a chosen parallel reduction strategy.
     * @param method 0 = OpenMP reduce clause, 1 = atomic accumulation
     */
    void calculateEnergy(int method);

    /**
     * @param method     0 = reduce, 1 = atomic
     * @param use_private If true, uses thread-private accumulators before a
     *                    final critical reduction (illustrates private/shared).
     */
    void calculateEnergy(int method, bool use_private);

    /**
     * Computes per-body metrics using firstprivate accumulators
     * (illustrates firstprivate clause).
     */
    void calculateMetricsFirstprivate();

    /**
     * Captures final-step state using lastprivate (illustrates lastprivate clause).
     */
    void calculateFinalStateLastprivate();

    // -----------------------------------------------------------------------
    // Getters — valid after a compute call
    // -----------------------------------------------------------------------
    double getKineticEnergy()   const;
    double getPotentialEnergy() const;
    double getTotalEnergy()     const;
    double getMomentumX()       const;
    double getMomentumY()       const;
    double getMomentumMag()     const;
    double getCenterMassX()     const;
    double getCenterMassY()     const;
    double getRmsRadius()       const;
    double getMinDistance()     const;

    /** Prints a formatted summary of all cached metrics to stdout. */
    void printSummary() const;
};