#include "Integrator.h"

/**
 * @brief Performs one explicit Euler step on the entire N-body system.
 *
 * Step order (must NOT be interleaved — see PDF §4.2 and §4.2.1):
 *   1. computeAccelerations() — evaluates all pairwise forces from CURRENT positions.
 *   2. kick() on every particle — updates ALL velocities: v_i <- v_i + a_i * dt.
 *   3. drift() on every particle — updates ALL positions: r_i <- r_i + v_i * dt.
 *
 * Interleaving kicks and drifts would corrupt the force evaluation order
 * and produce incorrect trajectories. The loops are kept separate intentionally.
 *
 * Week 1: uses the serial NBodySystem::computeAccelerations().
 * Week 2: NBodySimulator::integrateEuler() overloads will call the parallel
 *         computeAccelerations() variants and may add OpenMP directives here.
 *
 * @param sys  The N-body system to advance (modified in place).
 * @param dt   Time step size Δt. Stability requires dt << T_dyn where
 *             T_dyn ~ sqrt(L^3 / (G * M)) is the dynamical time of the system.
 */
void Integrator::eulerStep(NBodySystem& sys, double dt) {
    // Phase 1: compute all accelerations from current positions
    sys.computeAccelerations();

    // Phase 2: kick — update all velocities with the freshly computed accelerations
    for (Particle& p : sys.getBodies()) {
        p.kick(dt);
    }

    // Phase 3: drift — update all positions with the newly updated velocities
    for (Particle& p : sys.getBodies()) {
        p.drift(dt);
    }
}