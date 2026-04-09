#pragma once

#include "NBodySystem.h"

/**
 * @brief Explicit time integrators for the N-body simulation.
 *
 * Provides static methods for advancing the system by one time step.
 *
 * Week 1: eulerStep() stub — declared and minimally implemented.
 * Week 3: Full Euler implementation with OpenMP variants (integrateEuler overloads
 *         will live in NBodySimulator; this class handles the pure physics update).
 *
 * Euler step convention (per PDF §4.2):
 *   1. Compute ALL accelerations from current positions (NBodySystem::computeAccelerations)
 *   2. Kick ALL velocities:  v_i <- v_i + a_i * dt
 *   3. Drift ALL positions:  r_i <- r_i + v_i * dt
 *
 * The kick and drift must NOT be interleaved: finish all kicks before any drift.
 */
class Integrator {
public:
    /**
     * @brief Performs one explicit Euler step on the entire system.
     *
     * Calls sys.computeAccelerations(), then kicks all velocities,
     * then drifts all positions.
     *
     * @param sys  The N-body system to advance (modified in place)
     * @param dt   Time step size (should satisfy stability criteria, see README)
     */
    static void eulerStep(NBodySystem& sys, double dt);

    // -----------------------------------------------------------------------
    // Week 3 additions (optional higher-order integrators):
    // -----------------------------------------------------------------------
    // static void verletStep(NBodySystem& sys, double dt);
    // static void leapfrogStep(NBodySystem& sys, double dt);
};