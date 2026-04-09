#pragma once

#include "Particle.h"
#include <vector>

/**
 * @brief Container for all particles in the 2D N-body gravitational simulation.
 *
 * Manages a collection of Particle objects along with the gravitational constant G
 * and the Plummer softening parameter epsilon. Provides the core all-pairs force
 * computation used by the integrator each time step.
 *
 * Gravitational acceleration on body i (softened Newtonian gravity):
 *
 *   a_i = G * sum_{j != i}  m_j * (r_j - r_i) / (||r_j - r_i||^2 + eps^2)^(3/2)
 *
 * Unit convention: G = 1.0 in an adimensional system (see README for full declaration).
 * Softening eps > 0 avoids the singularity when two particles coincide.
 *
 * Week 1 : serial implementation of computeAccelerations().
 * Week 2 : parallel overloads with OpenMP scheduling and collapse will be added.
 */
class NBodySystem {
private:
    std::vector<Particle> bodies;   ///< All particles in the system
    double G_const;                 ///< Gravitational constant
    double softening_eps;           ///< Plummer softening parameter (epsilon)

public:
    /**
     * @param G       Gravitational constant (1.0 for adimensional units).
     * @param epsilon Softening parameter. Must be >= 0; use > 0 for real simulations.
     */
    NBodySystem(double G, double epsilon);

    // -----------------------------------------------------------------------
    // Particle management
    // -----------------------------------------------------------------------

    /** Appends a copy of particle p to the system. */
    void addParticle(const Particle& p);

    /** Sets every particle's acceleration to (0, 0).
     *  Must be called before accumulating forces for a new time step. */
    void zeroAccelerations();

    // -----------------------------------------------------------------------
    // Week 1 — Serial O(N^2) all-pairs computation
    // -----------------------------------------------------------------------

    /**
     * Computes pairwise gravitational accelerations for all bodies (serial).
     *
     * Implementation strategy:
     *   - Outer loop over body i writes only to bodies[i].acceleration.
     *   - Inner loop over j != i reads shared positions (read-only during accumulation).
     *   - zeroAccelerations() is called internally at the start.
     *
     * This is the reference baseline. Week 2 will add OpenMP overloads that
     * parallelise the outer loop over i with various schedule clauses.
     */
    void computeAccelerations();

    // -----------------------------------------------------------------------
    // Week 2 — Parallel overloads (OpenMP) — declared here, implemented later
    // -----------------------------------------------------------------------
    //
    // void computeAccelerations(int schedule_type);
    //     schedule_type: 0 = static, 1 = dynamic, 2 = guided
    //
    // void computeAccelerations(int schedule_type, int chunk_size);
    //     Same as above but with an explicit chunk size.
    //
    // void computeAccelerationsCollapse();
    //     Uses collapse(2) on the i,j nested loops (requires care with race conditions).

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------

    /** Read-only access to the particle vector. */
    const std::vector<Particle>& getBodies() const;

    /** Mutable access to the particle vector (used by the integrator). */
    std::vector<Particle>& getBodies();

    /** Number of particles in the system. */
    int getCount() const;

    /** Returns the gravitational constant G. */
    double getG() const;

    /** Returns the softening parameter epsilon. */
    double getEpsilon() const;
};