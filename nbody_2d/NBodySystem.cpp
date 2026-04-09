#include "NBodySystem.h"
#include <cmath>
#include <stdexcept>

// =============================================================================
// Constructor
// =============================================================================

NBodySystem::NBodySystem(double G, double epsilon)
    : G_const(G), softening_eps(epsilon)
{
    if (G < 0.0)
        throw std::invalid_argument("NBodySystem: gravitational constant G must be >= 0");
    if (epsilon < 0.0)
        throw std::invalid_argument("NBodySystem: softening epsilon must be >= 0");
}

// =============================================================================
// Particle management
// =============================================================================

void NBodySystem::addParticle(const Particle& p)
{
    if (p.getMass() <= 0.0)
        throw std::invalid_argument("NBodySystem::addParticle: mass must be > 0");
    bodies.push_back(p);
}

void NBodySystem::zeroAccelerations()
{
    for (Particle& p : bodies)
        p.zeroAcceleration();
}

// =============================================================================
// Week 1 — Serial O(N^2) all-pairs gravitational acceleration
// =============================================================================

/**
 * Computes pairwise gravitational accelerations for all bodies (serial reference).
 *
 * For each body i, accumulates contributions from every j != i:
 *
 *   a_i = G * sum_{j != i}  m_j * (r_j - r_i) / (||r_j - r_i||^2 + eps^2)^(3/2)
 *
 * Implementation notes
 * --------------------
 *   - zeroAccelerations() is called first so old values are discarded.
 *   - The outer loop iterates over i (the body being accelerated).
 *   - The inner loop iterates over j (source of gravity).
 *   - Each outer iteration accumulates into a LOCAL variable (aix, aiy) and
 *     writes to bodies[i] only once at the end.  This is the pattern that
 *     Week 2 will parallelise with "#pragma omp parallel for" on i: each
 *     thread owns its aix/aiy and reads shared positions (read-only), so
 *     there are no write race conditions.
 *   - The softened distance denominator is computed as:
 *       r2    = dx*dx + dy*dy + eps^2
 *       denom = r2^(3/2)              [via r2 * sqrt(r2)]
 *     Avoiding pow() improves performance and numerical consistency.
 *
 * Complexity: O(N^2) per call — one call per time step.
 *
 * Numerical example (from PDF §4.2):
 *   m1 at (0,0), m2 at (1,0), G=1, eps=0.1
 *   a1x = 1 * 1 * 1 / (1 + 0.01)^1.5 = 1 / 1.01^1.5 ≈ 0.9852
 */
void NBodySystem::computeAccelerations()
{
    zeroAccelerations();

    const int    n    = static_cast<int>(bodies.size());
    const double eps2 = softening_eps * softening_eps;

    for (int i = 0; i < n; i++) {
        const double xi = bodies[i].getX();
        const double yi = bodies[i].getY();

        // Local accumulators for body i — avoids repeated addAcceleration() calls
        // and is the pattern ready for OpenMP parallelisation in Week 2.
        double aix = 0.0;
        double aiy = 0.0;

        for (int j = 0; j < n; j++) {
            if (i == j) continue;

            const double dx = bodies[j].getX() - xi;
            const double dy = bodies[j].getY() - yi;

            // Softened squared distance: ||r_j - r_i||^2 + eps^2
            const double r2 = dx * dx + dy * dy + eps2;

            // Denominator: (r2)^(3/2)  =  r2 * sqrt(r2)
            const double r3 = r2 * std::sqrt(r2);

            // Scalar factor: G * m_j / r3
            const double factor = G_const * bodies[j].getMass() / r3;

            aix += factor * dx;
            aiy += factor * dy;
        }

        // Write accumulated acceleration to body i exactly once
        bodies[i].setAcceleration(aix, aiy);
    }
}

// =============================================================================
// Accessors
// =============================================================================

const std::vector<Particle>& NBodySystem::getBodies() const { return bodies; }
std::vector<Particle>&       NBodySystem::getBodies()       { return bodies; }
int    NBodySystem::getCount()   const { return static_cast<int>(bodies.size()); }
double NBodySystem::getG()       const { return G_const; }
double NBodySystem::getEpsilon() const { return softening_eps; }