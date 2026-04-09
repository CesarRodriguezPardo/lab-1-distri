#include "MetricsCalculator.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <limits>

// =============================================================================
// Constructor
// =============================================================================

MetricsCalculator::MetricsCalculator(const NBodySystem* sys)
    : system(sys),
      kineticEnergy(0.0),
      potentialEnergy(0.0),
      totalEnergy(0.0),
      momentumX(0.0),
      momentumY(0.0),
      centerMassX(0.0),
      centerMassY(0.0),
      rmsRadius(0.0),
      minDistance(0.0)
{
    if (!sys)
        throw std::invalid_argument("MetricsCalculator: system pointer must not be null");
}

// =============================================================================
// Week 3 — Serial reference implementations
// =============================================================================

/**
 * Computes kinetic energy:
 *   K = (1/2) * sum_i  m_i * (vx_i^2 + vy_i^2)
 */
double MetricsCalculator::computeKineticEnergy() const
{
    // TODO (Week 3): implement serial K computation
    double K = 0.0;
    for (const Particle& p : system->getBodies()) {
        double v2 = p.getVx() * p.getVx() + p.getVy() * p.getVy();
        K += 0.5 * p.getMass() * v2;
    }
    return K;
}

/**
 * Computes potential energy (softened):
 *   U = -G * sum_{i<j}  m_i * m_j / sqrt(||r_j - r_i||^2 + eps^2)
 */
double MetricsCalculator::computePotentialEnergy() const
{
    // TODO (Week 3): implement serial U computation
    double U = 0.0;
    const int n       = system->getCount();
    const double G    = system->getG();
    const double eps2 = system->getEpsilon() * system->getEpsilon();
    const std::vector<Particle>& bodies = system->getBodies();

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = bodies[j].getX() - bodies[i].getX();
            double dy = bodies[j].getY() - bodies[i].getY();
            double r  = std::sqrt(dx * dx + dy * dy + eps2);
            U -= G * bodies[i].getMass() * bodies[j].getMass() / r;
        }
    }
    return U;
}

/**
 * Computes all observables and caches them for getter access.
 */
void MetricsCalculator::computeAll()
{
    // TODO (Week 3): full serial implementation
    const std::vector<Particle>& bodies = system->getBodies();
    const int    n    = system->getCount();
    const double G    = system->getG();
    const double eps2 = system->getEpsilon() * system->getEpsilon();

    if (n == 0) return;

    // --- Kinetic energy, momentum, center of mass (single pass) ---
    double K    = 0.0;
    double px   = 0.0, py = 0.0;
    double cmx  = 0.0, cmy = 0.0;
    double M    = 0.0;

    for (const Particle& p : bodies) {
        double m  = p.getMass();
        double v2 = p.getVx() * p.getVx() + p.getVy() * p.getVy();
        K   += 0.5 * m * v2;
        px  += m * p.getVx();
        py  += m * p.getVy();
        cmx += m * p.getX();
        cmy += m * p.getY();
        M   += m;
    }
    cmx /= M;
    cmy /= M;

    // --- Potential energy and minimum distance (O(N^2) pairs) ---
    double U    = 0.0;
    double dmin = std::numeric_limits<double>::max();

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = bodies[j].getX() - bodies[i].getX();
            double dy = bodies[j].getY() - bodies[i].getY();
            double r2 = dx * dx + dy * dy;
            double r  = std::sqrt(r2 + eps2);
            U -= G * bodies[i].getMass() * bodies[j].getMass() / r;
            if (std::sqrt(r2) < dmin) dmin = std::sqrt(r2);
        }
    }

    // --- RMS radius relative to center of mass ---
    double rms = 0.0;
    for (const Particle& p : bodies) {
        double dx = p.getX() - cmx;
        double dy = p.getY() - cmy;
        rms += p.getMass() * (dx * dx + dy * dy);
    }
    rms = std::sqrt(rms / M);

    // --- Cache results ---
    kineticEnergy   = K;
    potentialEnergy = U;
    totalEnergy     = K + U;
    momentumX       = px;
    momentumY       = py;
    centerMassX     = cmx;
    centerMassY     = cmy;
    rmsRadius       = rms;
    minDistance     = (n > 1) ? dmin : 0.0;
}

// =============================================================================
// Week 3 — OpenMP parallel variants (stubs — full implementation in Week 3)
// =============================================================================

void MetricsCalculator::calculateEnergy(int method)
{
    // TODO (Week 3): parallel energy reduction
    //   method 0 → #pragma omp parallel for reduction(+:K, U)
    //   method 1 → atomic accumulation
    (void)method;
    computeAll();  // fallback to serial until Week 3
}

void MetricsCalculator::calculateEnergy(int method, bool use_private)
{
    // TODO (Week 3): parallel with thread-private accumulators
    //   use_private=true → each thread keeps local K/U, merges with critical
    (void)method;
    (void)use_private;
    computeAll();  // fallback to serial until Week 3
}

void MetricsCalculator::calculateMetricsFirstprivate()
{
    // TODO (Week 3): illustrates firstprivate clause
    //   An accumulator is initialised from the enclosing scope value (firstprivate)
    //   and each thread accumulates its own partial sum before a final reduction.
    computeAll();  // fallback to serial until Week 3
}

void MetricsCalculator::calculateFinalStateLastprivate()
{
    // TODO (Week 3): illustrates lastprivate clause
    //   The value of the loop variable or a per-iteration result from the
    //   last logical iteration survives after the parallel region.
    computeAll();  // fallback to serial until Week 3
}

// =============================================================================
// Getters
// =============================================================================

double MetricsCalculator::getKineticEnergy()   const { return kineticEnergy;   }
double MetricsCalculator::getPotentialEnergy()  const { return potentialEnergy; }
double MetricsCalculator::getTotalEnergy()      const { return totalEnergy;     }
double MetricsCalculator::getMomentumX()        const { return momentumX;       }
double MetricsCalculator::getMomentumY()        const { return momentumY;       }
double MetricsCalculator::getMomentumMag()      const {
    return std::sqrt(momentumX * momentumX + momentumY * momentumY);
}
double MetricsCalculator::getCenterMassX()      const { return centerMassX;     }
double MetricsCalculator::getCenterMassY()      const { return centerMassY;     }
double MetricsCalculator::getRmsRadius()        const { return rmsRadius;       }
double MetricsCalculator::getMinDistance()      const { return minDistance;     }

// =============================================================================
// Summary printer
// =============================================================================

void MetricsCalculator::printSummary() const
{
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  K          = " << kineticEnergy   << "\n";
    std::cout << "  U          = " << potentialEnergy << "\n";
    std::cout << "  E = K+U    = " << totalEnergy     << "\n";
    std::cout << "  |P|        = " << getMomentumMag() << "\n";
    std::cout << "  R_cm       = (" << centerMassX << ", " << centerMassY << ")\n";
    std::cout << "  R_rms      = " << rmsRadius    << "\n";
    std::cout << "  d_min      = " << minDistance  << "\n";
}