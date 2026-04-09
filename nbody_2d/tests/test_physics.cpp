/**
 * @file tests/test_physics.cpp
 * @brief Unit and integration tests — GoogleTest suite.
 *
 * Test suites
 * -----------
 *   ParticleTest          — construction, kick, drift, acceleration ops
 *   NBodySystemTest       — particle management, accessors, input guards
 *   ForceComputationTest  — N=2 analytical cases (with and without softening)
 *   NewtonThirdLawTest    — F_ij = -F_ji for equal and unequal masses
 *   N3SuperpositionTest   — N=3 case verified analytically by hand (PDF §4.2)
 *   IntegratorTest        — eulerStep order, attraction, momentum conservation
 *   RegressionTest        — idempotency, mass sensitivity, no-write corruption
 *
 * Floating-point tolerance justification
 * ----------------------------------------
 * All computations use IEEE-754 double precision (ε_machine ≈ 2.22e-16).
 * The force kernel involves: 2 subtractions, 2 multiplications, 1 sqrt,
 * 1 division, and 1 FMA-like accumulation per pair.  For N ≤ 3 bodies with
 * O(1) coordinates and masses, the accumulated rounding error is bounded by
 *
 *   |err| ≲ C · N · ε_machine ≈ 3 × 5 × 2.22e-16 ≈ 3.3e-15
 *
 * We adopt TOL = 1e-10 as absolute tolerance for single-pair comparisons —
 * five orders of magnitude above the theoretical rounding floor.  This
 * avoids false negatives from FP non-associativity while catching real bugs.
 * Tests with quantities of magnitude ~1e10 use a scaled relative tolerance
 * (|expected| × 1e-12) to stay consistent with double-precision limits.
 *
 * Compilation (handled by Makefile):
 *   g++ -Wall -Wextra -O3 -fopenmp -std=c++17 \
 *       tests/test_physics.cpp Particle.cpp NBodySystem.cpp Integrator.cpp \
 *       -lgtest -lgtest_main -fopenmp -o run_tests
 */

#include <gtest/gtest.h>

#include "../Particle.h"
#include "../NBodySystem.h"
#include "../Integrator.h"

#include <cmath>
#include <stdexcept>

// =============================================================================
// Shared tolerance (see justification in file header)
// =============================================================================
static constexpr double TOL = 1e-10;

// =============================================================================
// Helper: exact softened acceleration component
//   a_i_comp = G * mj * comp / (dx^2 + dy^2 + eps^2)^(3/2)
// =============================================================================
static double softened_acc(double G, double mj,
                            double dx, double dy,
                            double eps, double comp)
{
    double r2 = dx*dx + dy*dy + eps*eps;
    return G * mj * comp / (r2 * std::sqrt(r2));
}

// =============================================================================
// 1. ParticleTest
// =============================================================================

TEST(ParticleTest, DefaultAccelerationIsZero)
{
    Particle p(1.0, 3.0, -2.0, 0.5, -0.5);
    EXPECT_DOUBLE_EQ(p.getAx(), 0.0);
    EXPECT_DOUBLE_EQ(p.getAy(), 0.0);
}

TEST(ParticleTest, GettersReturnConstructionValues)
{
    Particle p(2.5, 1.0, -3.0, 4.0, -5.0);
    EXPECT_DOUBLE_EQ(p.getMass(), 2.5);
    EXPECT_DOUBLE_EQ(p.getX(),    1.0);
    EXPECT_DOUBLE_EQ(p.getY(),   -3.0);
    EXPECT_DOUBLE_EQ(p.getVx(),   4.0);
    EXPECT_DOUBLE_EQ(p.getVy(),  -5.0);
}

TEST(ParticleTest, SettersWork)
{
    Particle p(1.0, 0.0, 0.0);
    p.setPosition(7.0, -8.0);
    p.setVelocity(-1.5, 2.5);
    EXPECT_DOUBLE_EQ(p.getX(),   7.0);
    EXPECT_DOUBLE_EQ(p.getY(),  -8.0);
    EXPECT_DOUBLE_EQ(p.getVx(), -1.5);
    EXPECT_DOUBLE_EQ(p.getVy(),  2.5);
}

TEST(ParticleTest, ZeroAccelerationClearsValue)
{
    Particle p(1.0, 0.0, 0.0);
    p.addAcceleration(3.14, -2.71);
    p.zeroAcceleration();
    EXPECT_DOUBLE_EQ(p.getAx(), 0.0);
    EXPECT_DOUBLE_EQ(p.getAy(), 0.0);
}

TEST(ParticleTest, AddAccelerationAccumulates)
{
    Particle p(1.0, 0.0, 0.0);
    p.addAcceleration(1.0,  2.0);
    p.addAcceleration(0.5, -3.0);
    EXPECT_DOUBLE_EQ(p.getAx(),  1.5);
    EXPECT_DOUBLE_EQ(p.getAy(), -1.0);
}

TEST(ParticleTest, SetAccelerationOverwrites)
{
    Particle p(1.0, 0.0, 0.0);
    p.addAcceleration(100.0, 200.0);
    p.setAcceleration(-1.0, 5.0);
    EXPECT_DOUBLE_EQ(p.getAx(), -1.0);
    EXPECT_DOUBLE_EQ(p.getAy(),  5.0);
}

TEST(ParticleTest, KickUpdatesVelocity)
{
    // v_new = v + a * dt
    // vx = 2.0 + 3.0*0.5 = 3.5
    // vy = -1.0 + (-4.0)*0.5 = -3.0
    Particle p(1.0, 0.0, 0.0, 2.0, -1.0);
    p.setAcceleration(3.0, -4.0);
    p.kick(0.5);
    EXPECT_DOUBLE_EQ(p.getVx(),  3.5);
    EXPECT_DOUBLE_EQ(p.getVy(), -3.0);
}

TEST(ParticleTest, DriftUpdatesPosition)
{
    // r_new = r + v * dt
    // x = 1.0 + 3.0*0.5 = 2.5
    // y = 2.0 + 4.0*0.5 = 4.0
    Particle p(1.0, 1.0, 2.0, 3.0, 4.0);
    p.drift(0.5);
    EXPECT_DOUBLE_EQ(p.getX(), 2.5);
    EXPECT_DOUBLE_EQ(p.getY(), 4.0);
}

TEST(ParticleTest, KickThenDriftEulerPattern)
{
    // Lone particle: a=(1,0), v0=(0,0), dt=0.1
    // kick:  vx = 0 + 1*0.1 = 0.1
    // drift: x  = 0 + 0.1*0.1 = 0.01
    Particle p(1.0, 0.0, 0.0, 0.0, 0.0);
    p.setAcceleration(1.0, 0.0);
    p.kick(0.1);
    p.drift(0.1);
    EXPECT_DOUBLE_EQ(p.getVx(), 0.1);
    EXPECT_DOUBLE_EQ(p.getX(),  0.01);
}

// =============================================================================
// 2. NBodySystemTest — management and accessors
// =============================================================================

TEST(NBodySystemTest, ZeroAccelerationsClearsAll)
{
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 1.0, 0.0));
    sys.getBodies()[0].addAcceleration(99.0, 99.0);
    sys.zeroAccelerations();
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAx(), 0.0);
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAy(), 0.0);
    EXPECT_DOUBLE_EQ(sys.getBodies()[1].getAx(), 0.0);
    EXPECT_DOUBLE_EQ(sys.getBodies()[1].getAy(), 0.0);
}

TEST(NBodySystemTest, GetCountIsCorrect)
{
    NBodySystem sys(1.0, 0.1);
    EXPECT_EQ(sys.getCount(), 0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    EXPECT_EQ(sys.getCount(), 1);
    sys.addParticle(Particle(1.0, 1.0, 0.0));
    EXPECT_EQ(sys.getCount(), 2);
}

TEST(NBodySystemTest, AccessorsReturnConstructorValues)
{
    NBodySystem sys(6.674e-11, 0.05);
    EXPECT_DOUBLE_EQ(sys.getG(),       6.674e-11);
    EXPECT_DOUBLE_EQ(sys.getEpsilon(), 0.05);
}

// =============================================================================
// 3. NBodySystemTest — input guards (invalid arguments)
// =============================================================================

TEST(NBodySystemTest, RejectsZeroMass)
{
    NBodySystem sys(1.0, 0.1);
    EXPECT_THROW(sys.addParticle(Particle(0.0, 0.0, 0.0)), std::invalid_argument);
}

TEST(NBodySystemTest, RejectsNegativeMass)
{
    NBodySystem sys(1.0, 0.1);
    EXPECT_THROW(sys.addParticle(Particle(-1.0, 0.0, 0.0)), std::invalid_argument);
}

TEST(NBodySystemTest, RejectsNegativeEpsilon)
{
    EXPECT_THROW({ NBodySystem sys(1.0, -0.1); (void)sys; }, std::invalid_argument);
}

TEST(NBodySystemTest, RejectsNegativeG)
{
    EXPECT_THROW({ NBodySystem sys(-1.0, 0.1); (void)sys; }, std::invalid_argument);
}

// =============================================================================
// 4. ForceComputationTest — N=2 analytical cases
// =============================================================================

TEST(ForceComputationTest, SingleBodyHasZeroAcceleration)
{
    // No other body → no pairwise force
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(5.0, 3.0, -2.0, 1.0, 1.0));
    sys.computeAccelerations();
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAx(), 0.0);
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAy(), 0.0);
}

TEST(ForceComputationTest, N2AlongXAxisNoSoftening)
{
    // d = 5, G = m = 1, eps = 0
    // a0x = G*m*dx / dx^3 = 1/25 = 0.04
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 5.0, 0.0));
    sys.computeAccelerations();

    EXPECT_NEAR(sys.getBodies()[0].getAx(),  1.0/25.0, TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  0.0,      TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAx(), -1.0/25.0, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(),  0.0,      TOL);
}

TEST(ForceComputationTest, N2DiagonalThreeFourFiveTriangle)
{
    // Bodies at (0,0) and (3,4), distance = 5, r^3 = 125, G=m=1, eps=0
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 3.0, 4.0));
    sys.computeAccelerations();

    EXPECT_NEAR(sys.getBodies()[0].getAx(),  3.0/125.0, TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  4.0/125.0, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAx(), -3.0/125.0, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(), -4.0/125.0, TOL);
}

TEST(ForceComputationTest, N2SoftenedPDFExample)
{
    // PDF §4.2 numerical example:
    //   m1 at (0,0), m2 at (1,0), G=1, eps=0.1
    //   a1x = 1 / (1 + 0.01)^(3/2) = 1 / 1.01^1.5 ≈ 0.98520
    const double G = 1.0, eps = 0.1, d = 1.0, m = 1.0;
    NBodySystem sys(G, eps);
    sys.addParticle(Particle(m, 0.0, 0.0));
    sys.addParticle(Particle(m, d,   0.0));
    sys.computeAccelerations();

    const double expected = softened_acc(G, m, d, 0.0, eps, d);
    EXPECT_NEAR(sys.getBodies()[0].getAx(),  expected, TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  0.0,      TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAx(), -expected, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(),  0.0,      TOL);
}

TEST(ForceComputationTest, N2SoftenedAlongYAxis)
{
    const double G = 2.0, eps = 0.5, dy = 3.0, m = 4.0;
    NBodySystem sys(G, eps);
    sys.addParticle(Particle(m, 0.0, 0.0));
    sys.addParticle(Particle(m, 0.0, dy));
    sys.computeAccelerations();

    const double expected_y = softened_acc(G, m, 0.0, dy, eps, dy);
    EXPECT_NEAR(sys.getBodies()[0].getAx(),  0.0,        TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  expected_y, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAx(),  0.0,        TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(), -expected_y, TOL);
}

TEST(ForceComputationTest, SymmetricConfigurationAntiSymmetricForce)
{
    // Bodies at (-d, 0) and (+d, 0): ax must be equal and opposite
    const double d = 2.0, G = 1.0, eps = 0.3, m = 1.0;
    NBodySystem sys(G, eps);
    sys.addParticle(Particle(m, -d, 0.0));
    sys.addParticle(Particle(m,  d, 0.0));
    sys.computeAccelerations();

    EXPECT_NEAR(sys.getBodies()[0].getAx(), -sys.getBodies()[1].getAx(), TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  0.0, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(),  0.0, TOL);
}

// =============================================================================
// 5. NewtonThirdLawTest — F_ij = -F_ji  (PDF §10)
// =============================================================================

TEST(NewtonThirdLawTest, EqualMasses)
{
    // m1*a1 + m2*a2 = 0  (total internal force is zero)
    NBodySystem sys(1.0, 0.2);
    sys.addParticle(Particle(3.0, 0.0, 0.0));
    sys.addParticle(Particle(3.0, 2.0, 1.5));
    sys.computeAccelerations();

    const double m0 = sys.getBodies()[0].getMass();
    const double m1 = sys.getBodies()[1].getMass();
    EXPECT_NEAR(m0 * sys.getBodies()[0].getAx() + m1 * sys.getBodies()[1].getAx(), 0.0, TOL);
    EXPECT_NEAR(m0 * sys.getBodies()[0].getAy() + m1 * sys.getBodies()[1].getAy(), 0.0, TOL);
}

TEST(NewtonThirdLawTest, UnequalMasses)
{
    // F = m*a must satisfy F_01 = -F_10 even when masses differ greatly
    const double m0 = 1e10, m1 = 5e8;
    NBodySystem sys(6.674e-11, 1e-3);
    sys.addParticle(Particle(m0,  0.0, 0.0));
    sys.addParticle(Particle(m1, 10.0, 5.0));
    sys.computeAccelerations();

    const double F0x = m0 * sys.getBodies()[0].getAx();
    const double F0y = m0 * sys.getBodies()[0].getAy();
    const double F1x = m1 * sys.getBodies()[1].getAx();
    const double F1y = m1 * sys.getBodies()[1].getAy();

    // Use scaled tolerance: relative to the magnitude of the force
    EXPECT_NEAR(F0x + F1x, 0.0, std::abs(F0x) * 1e-12 + 1e-30);
    EXPECT_NEAR(F0y + F1y, 0.0, std::abs(F0y) * 1e-12 + 1e-30);
}

// =============================================================================
// 6. N3SuperpositionTest — hand-verified analytical case (PDF §4.2)
// =============================================================================

TEST(N3SuperpositionTest, AccelerationsMatchAnalytical)
{
    // Configuration: m=1 at (0,0), (1,0), (0,1); G=1, eps=0
    //
    // Body 0 contributions:
    //   from 1: dx=+1, dy= 0, r=1  → a0x += 1,          a0y += 0
    //   from 2: dx= 0, dy=+1, r=1  → a0x += 0,          a0y += 1
    //   => a0 = (1, 1)
    //
    // Body 1 contributions:
    //   from 0: dx=-1, dy= 0, r=1        → a1x += -1,         a1y += 0
    //   from 2: dx=-1, dy=+1, r=√2       → a1x += -1/(2√2),   a1y += 1/(2√2)
    //   => a1 = (-1 - 1/(2√2),  1/(2√2))
    //
    // Body 2 contributions:
    //   from 0: dx= 0, dy=-1, r=1        → a2x += 0,           a2y += -1
    //   from 1: dx=+1, dy=-1, r=√2       → a2x += 1/(2√2),     a2y += -1/(2√2)
    //   => a2 = (1/(2√2), -1 - 1/(2√2))
    //
    // Conservation check: sum(a_i) = 0 (equal masses) ✓
    const double inv2sq2 = 1.0 / (2.0 * std::sqrt(2.0)); // ≈ 0.35355

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 1.0, 0.0));
    sys.addParticle(Particle(1.0, 0.0, 1.0));
    sys.computeAccelerations();

    // Body 0
    EXPECT_NEAR(sys.getBodies()[0].getAx(),  1.0,           TOL);
    EXPECT_NEAR(sys.getBodies()[0].getAy(),  1.0,           TOL);
    // Body 1
    EXPECT_NEAR(sys.getBodies()[1].getAx(), -1.0 - inv2sq2, TOL);
    EXPECT_NEAR(sys.getBodies()[1].getAy(),  inv2sq2,        TOL);
    // Body 2
    EXPECT_NEAR(sys.getBodies()[2].getAx(),  inv2sq2,        TOL);
    EXPECT_NEAR(sys.getBodies()[2].getAy(), -1.0 - inv2sq2,  TOL);
}

TEST(N3SuperpositionTest, TotalInternalForceIsZero)
{
    // sum(m_i * a_i) = 0 for any configuration (only internal forces)
    NBodySystem sys(1.0, 0.05);
    sys.addParticle(Particle(2.0,  0.0,  0.0));
    sys.addParticle(Particle(3.0,  1.0,  2.0));
    sys.addParticle(Particle(1.5, -1.5,  0.5));
    sys.computeAccelerations();

    double Fx = 0.0, Fy = 0.0, scale = 0.0;
    for (const Particle& p : sys.getBodies()) {
        Fx    += p.getMass() * p.getAx();
        Fy    += p.getMass() * p.getAy();
        scale += p.getMass() * (std::abs(p.getAx()) + std::abs(p.getAy()));
    }
    EXPECT_NEAR(Fx, 0.0, scale * 1e-13 + 1e-30);
    EXPECT_NEAR(Fy, 0.0, scale * 1e-13 + 1e-30);
}

// =============================================================================
// 7. IntegratorTest  (PDF §10 — integration tests)
// =============================================================================

TEST(IntegratorTest, LoneBodyDriftsAtConstantVelocity)
{
    // 1 body, no forces: a=0, v constant, x_new = x0 + v*dt
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0, 1.0, 0.0, 1.0, 0.5));
    Integrator::eulerStep(sys, 0.1);

    EXPECT_NEAR(sys.getBodies()[0].getX(),  1.0 + 1.0*0.1, TOL);
    EXPECT_NEAR(sys.getBodies()[0].getY(),  0.0 + 0.5*0.1, TOL);
    EXPECT_NEAR(sys.getBodies()[0].getVx(), 1.0,            TOL); // unchanged
    EXPECT_NEAR(sys.getBodies()[0].getVy(), 0.5,            TOL); // unchanged
}

TEST(IntegratorTest, TwoBodiesAtRestAttractAfterOneStep)
{
    // Two bodies at rest: after one step they must move toward each other
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0, -1.0, 0.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0,  1.0, 0.0, 0.0, 0.0));
    Integrator::eulerStep(sys, 0.001);

    // Body 0 at x=-1 must be pulled right (toward body 1 at x=+1)
    EXPECT_GT(sys.getBodies()[0].getVx(), 0.0);
    EXPECT_GT(sys.getBodies()[0].getX(), -1.0);

    // Body 1 at x=+1 must be pulled left (toward body 0 at x=-1)
    EXPECT_LT(sys.getBodies()[1].getVx(), 0.0);
    EXPECT_LT(sys.getBodies()[1].getX(),  1.0);
}

TEST(IntegratorTest, EulerOrderIsAccKickDrift)
{
    // Verify the mandated step order (PDF §4.2.1):
    //   1. computeAccelerations  2. kick  3. drift
    //
    // Setup: body at rest at origin with another body at (1,0).
    // After ONE step the position update must use the POST-kick velocity,
    // not the pre-kick velocity (which is zero).
    // So x_new > 0  (moved in the direction of the acceleration).
    NBodySystem sys(1.0, 0.01);
    sys.addParticle(Particle(1.0, 0.0, 0.0, 0.0, 0.0)); // at rest
    sys.addParticle(Particle(1.0, 1.0, 0.0, 0.0, 0.0));
    Integrator::eulerStep(sys, 0.1);

    // Body 0 was at rest; after kick its velocity is > 0,
    // after drift its position must have moved right.
    EXPECT_GT(sys.getBodies()[0].getX(), 0.0)
        << "Position did not change — drift may be using pre-kick velocity";
}

TEST(IntegratorTest, LinearMomentumConservedOverMultipleSteps)
{
    // Total linear momentum P = sum(m_i * v_i) must be constant:
    // Euler is symplectic in the sense that symmetric force pairs
    // (F_ij = -F_ji) preserve the total momentum exactly.
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0,  1.0,  0.0,  0.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0,  0.0,  0.0, -1.0));
    sys.addParticle(Particle(2.0,  0.0,  1.0, -0.5,  0.0));

    // Initial total momentum
    double px0 = 0.0, py0 = 0.0;
    for (const Particle& p : sys.getBodies()) {
        px0 += p.getMass() * p.getVx();
        py0 += p.getMass() * p.getVy();
    }

    // Advance 10 steps
    for (int i = 0; i < 10; i++)
        Integrator::eulerStep(sys, 0.001);

    // Final total momentum
    double px1 = 0.0, py1 = 0.0;
    for (const Particle& p : sys.getBodies()) {
        px1 += p.getMass() * p.getVx();
        py1 += p.getMass() * p.getVy();
    }

    EXPECT_NEAR(px1, px0, 1e-12);
    EXPECT_NEAR(py1, py0, 1e-12);
}

// =============================================================================
// 8. RegressionTest  (PDF §10 — regression cases)
// =============================================================================

TEST(RegressionTest, ForceIsSensitiveToMass)
{
    // Doubling the mass of body 1 must double body 0's acceleration.
    // Guards against implementations that ignore mass in the kernel.
    NBodySystem sys1(1.0, 0.1), sys2(1.0, 0.1);
    sys1.addParticle(Particle(1.0, 0.0, 0.0));
    sys1.addParticle(Particle(1.0, 1.0, 0.0));
    sys2.addParticle(Particle(1.0, 0.0, 0.0));
    sys2.addParticle(Particle(2.0, 1.0, 0.0)); // double mass

    sys1.computeAccelerations();
    sys2.computeAccelerations();

    EXPECT_NEAR(sys2.getBodies()[0].getAx(),
                2.0 * sys1.getBodies()[0].getAx(), TOL);
}

TEST(RegressionTest, ComputeAccelerationsIsIdempotent)
{
    // Calling computeAccelerations() twice must yield identical results.
    // Guards against stale accumulation bugs (failure to zero first).
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0,  0.0,  0.0));
    sys.addParticle(Particle(2.0,  1.0,  0.5));
    sys.addParticle(Particle(0.5, -1.0,  1.0));

    sys.computeAccelerations();
    const double ax0_first = sys.getBodies()[0].getAx();
    const double ay0_first = sys.getBodies()[0].getAy();

    sys.computeAccelerations(); // second call
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAx(), ax0_first);
    EXPECT_DOUBLE_EQ(sys.getBodies()[0].getAy(), ay0_first);
}

TEST(RegressionTest, NoWriteCorruptionToOtherBodiesAcceleration)
{
    // After computeAccelerations(), every body must have a well-defined
    // (non-trivially-zero) acceleration when N>1 and bodies are not collinear.
    // Guards against loop bugs that skip writing some bodies.
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0,  1.0, 0.0));
    sys.addParticle(Particle(1.0,  0.0, 1.0));
    sys.computeAccelerations();

    for (int i = 0; i < sys.getCount(); i++) {
        const double ax = sys.getBodies()[i].getAx();
        const double ay = sys.getBodies()[i].getAy();
        EXPECT_FALSE(ax == 0.0 && ay == 0.0)
            << "Body " << i << " has (0,0) acceleration — possible write corruption";
    }
}

TEST(RegressionTest, SerialResultMatchesForDifferentEpsilon)
{
    // Changing epsilon must change the result (guards against eps being ignored).
    NBodySystem sysA(1.0, 0.0);  // no softening
    NBodySystem sysB(1.0, 1.0);  // large softening

    sysA.addParticle(Particle(1.0, 0.0, 0.0));
    sysA.addParticle(Particle(1.0, 1.0, 0.0));
    sysB.addParticle(Particle(1.0, 0.0, 0.0));
    sysB.addParticle(Particle(1.0, 1.0, 0.0));

    sysA.computeAccelerations();
    sysB.computeAccelerations();

    // sysA must have a stronger force than sysB (smaller eps → larger force)
    EXPECT_GT(sysA.getBodies()[0].getAx(), sysB.getBodies()[0].getAx())
        << "Softening parameter eps is not affecting the force computation";
}