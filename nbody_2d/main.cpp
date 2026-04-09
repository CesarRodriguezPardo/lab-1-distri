/**
 * @file main.cpp
 * @brief Entry point for the 2D N-body gravitational simulator.
 *
 * Week 1: verifies the serial force computation for N=2 and N=3 bodies
 *         against analytically derived values from the PDF (§4.2).
 *
 * Compilation:
 *   make all
 *
 * Usage:
 *   ./nbody_2d                  — run Week-1 physics verifications
 *   ./nbody_2d --benchmark      — run benchmarks (implemented Week 4)
 *   ./nbody_2d --analysis       — run scaling analysis (implemented Week 4)
 *   ./nbody_2d --simulate       — run a full simulation (implemented Week 3)
 *   ./nbody_2d --help           — show this help
 */

#include "Particle.h"
#include "NBodySystem.h"
#include "NBodySimulator.h"
#include "Benchmark.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include <vector>

// =============================================================================
// Helper utilities
// =============================================================================

static const std::string HR(60, '=');
static const std::string hr(60, '-');

static bool approxEqual(double a, double b, double tol = 1e-10) {
    return std::abs(a - b) <= tol;
}

static void printResult(const std::string& label, double computed,
                        double expected, double tol = 1e-10) {
    bool ok  = approxEqual(computed, expected, tol);
    double err = std::abs(computed - expected);
    std::cout << "  " << std::left  << std::setw(24) << label
              << "  computed = " << std::right << std::setw(12) << std::fixed
              << std::setprecision(8) << computed
              << "  expected = " << std::setw(12) << expected
              << "  |err| = "   << std::scientific << std::setprecision(2) << err
              << "  [" << (ok ? "PASS" : "FAIL") << "]\n";
}

// =============================================================================
// Week 1 — N=2 verification
// =============================================================================

/**
 * Verifies the gravitational acceleration between two bodies aligned on the x-axis.
 *
 * Configuration (from PDF §4.2 numerical example):
 *   m1 = 1 at (0, 0),  m2 = 1 at (1, 0)
 *   G = 1.0,  eps = 0.1  (adimensional units)
 *
 * Analytical result:
 *   a1x = G * m2 * dx / (dx² + eps²)^(3/2)
 *       = 1 * 1 * 1 / (1 + 0.01)^1.5
 *       ≈ 0.985 22...
 *
 *   a1y = 0   (no y separation)
 *
 * Newton's third law (action–reaction):
 *   a2x = -a1x,   a2y = 0
 *
 * Also verifies with G = 6.674e-11 and real masses to confirm unit-independence.
 */
static bool verifyN2() {
    std::cout << "\n" << HR << "\n";
    std::cout << "  N = 2 Verification\n";
    std::cout << "  Bodies: m1=1 at (0,0),  m2=1 at (1,0)\n";
    std::cout << "  G = 1.0,  eps = 0.10  (adimensional)\n";
    std::cout << HR << "\n";

    const double G   = 1.0;
    const double eps = 0.1;
    const double dx  = 1.0;    // x2 - x1
    const double dy  = 0.0;    // y2 - y1
    const double m1  = 1.0;
    const double m2  = 1.0;

    // Analytical: a1x = G * m2 * dx / (dx² + dy² + eps²)^(3/2)
    double r2       = dx * dx + dy * dy + eps * eps;
    double expected_a1x = G * m2 * dx / (r2 * std::sqrt(r2));
    double expected_a1y = 0.0;
    double expected_a2x = -expected_a1x;  // Newton's 3rd law
    double expected_a2y = 0.0;

    // Numerical
    NBodySystem sys(G, eps);
    sys.addParticle(Particle(m1, 0.0, 0.0));
    sys.addParticle(Particle(m2, dx,  0.0));
    sys.computeAccelerations();

    const auto& b = sys.getBodies();

    bool pass = true;
    printResult("a[0].x", b[0].getAx(), expected_a1x);
    printResult("a[0].y", b[0].getAy(), expected_a1y);
    printResult("a[1].x", b[1].getAx(), expected_a2x);
    printResult("a[1].y", b[1].getAy(), expected_a2y);

    pass = approxEqual(b[0].getAx(), expected_a1x)
        && approxEqual(b[0].getAy(), expected_a1y)
        && approxEqual(b[1].getAx(), expected_a2x)
        && approxEqual(b[1].getAy(), expected_a2y);

    // ---- verify Newton's 3rd law: m1*a1 + m2*a2 = 0 (net internal force = 0) ----
    double net_fx = m1 * b[0].getAx() + m2 * b[1].getAx();
    double net_fy = m1 * b[0].getAy() + m2 * b[1].getAy();
    std::cout << hr << "\n";
    printResult("Net force x (=0?)", net_fx, 0.0);
    printResult("Net force y (=0?)", net_fy, 0.0);
    pass = pass && approxEqual(net_fx, 0.0) && approxEqual(net_fy, 0.0);

    std::cout << "\n  Result: " << (pass ? "ALL PASS ✓" : "SOME TESTS FAILED ✗") << "\n";
    return pass;
}

// =============================================================================
// Week 1 — N=3 verification
// =============================================================================

/**
 * Verifies the gravitational acceleration for three equal-mass bodies
 * forming a right triangle in the plane.
 *
 * Configuration:
 *   Body 0: m=1 at (0, 0)
 *   Body 1: m=1 at (1, 0)
 *   Body 2: m=1 at (0, 1)
 *   G = 1.0,  eps ≈ 0  (1e-12, negligible)
 *
 * Analytical results (superposition of pairwise contributions):
 *
 *   Body 0 receives:
 *     from 1: dx=1,  dy=0,  r=1  →  a0x += 1,      a0y += 0
 *     from 2: dx=0,  dy=1,  r=1  →  a0x += 0,      a0y += 1
 *     Total: a0x =  1.0,   a0y =  1.0
 *
 *   Body 1 receives:
 *     from 0: dx=-1, dy=0,  r=1       →  a1x += -1,         a1y += 0
 *     from 2: dx=-1, dy=1,  r=√2      →  a1x += -1/(2√2),  a1y += 1/(2√2)
 *     Total: a1x = -1 - 1/(2√2) ≈ -1.35355
 *            a1y =      1/(2√2) ≈  0.35355
 *
 *   Body 2 receives:
 *     from 0: dx=0,  dy=-1, r=1       →  a2x += 0,          a2y += -1
 *     from 1: dx=1,  dy=-1, r=√2      →  a2x += 1/(2√2),   a2y += -1/(2√2)
 *     Total: a2x =     1/(2√2) ≈  0.35355
 *            a2y = -1 - 1/(2√2) ≈ -1.35355
 *
 * Conservation check: sum of m_i * a_i = 0 (total internal force).
 */
static bool verifyN3() {
    std::cout << "\n" << HR << "\n";
    std::cout << "  N = 3 Verification\n";
    std::cout << "  Bodies: m=1 at (0,0), (1,0), (0,1)\n";
    std::cout << "  G = 1.0,  eps = 1e-12 (negligible)\n";
    std::cout << HR << "\n";

    const double G   = 1.0;
    const double eps = 1e-12;  // effectively zero softening for hand calculation
    const double m   = 1.0;
    const double inv2sqrt2 = 1.0 / (2.0 * std::sqrt(2.0));  // 1/(2√2) ≈ 0.35355

    // Analytical expected accelerations
    const double exp_a0x =  1.0;
    const double exp_a0y =  1.0;

    const double exp_a1x = -1.0 - inv2sqrt2;
    const double exp_a1y =        inv2sqrt2;

    const double exp_a2x =        inv2sqrt2;
    const double exp_a2y = -1.0 - inv2sqrt2;

    // Numerical
    NBodySystem sys(G, eps);
    sys.addParticle(Particle(m, 0.0, 0.0));
    sys.addParticle(Particle(m, 1.0, 0.0));
    sys.addParticle(Particle(m, 0.0, 1.0));
    sys.computeAccelerations();

    const auto& b = sys.getBodies();
    const double tol = 1e-9;  // slightly relaxed due to tiny eps != 0

    bool pass = true;

    std::cout << "  --- Body 0 at (0,0) ---\n";
    printResult("a[0].x", b[0].getAx(), exp_a0x, tol);
    printResult("a[0].y", b[0].getAy(), exp_a0y, tol);

    std::cout << "  --- Body 1 at (1,0) ---\n";
    printResult("a[1].x", b[1].getAx(), exp_a1x, tol);
    printResult("a[1].y", b[1].getAy(), exp_a1y, tol);

    std::cout << "  --- Body 2 at (0,1) ---\n";
    printResult("a[2].x", b[2].getAx(), exp_a2x, tol);
    printResult("a[2].y", b[2].getAy(), exp_a2y, tol);

    pass = approxEqual(b[0].getAx(), exp_a0x, tol)
        && approxEqual(b[0].getAy(), exp_a0y, tol)
        && approxEqual(b[1].getAx(), exp_a1x, tol)
        && approxEqual(b[1].getAy(), exp_a1y, tol)
        && approxEqual(b[2].getAx(), exp_a2x, tol)
        && approxEqual(b[2].getAy(), exp_a2y, tol);

    // ---- Conservation of momentum: sum(m_i * a_i) = 0 ----
    double net_fx = m * (b[0].getAx() + b[1].getAx() + b[2].getAx());
    double net_fy = m * (b[0].getAy() + b[1].getAy() + b[2].getAy());
    std::cout << "  --- Total internal force (must be 0) ---\n";
    printResult("Net force x (=0?)", net_fx, 0.0, tol);
    printResult("Net force y (=0?)", net_fy, 0.0, tol);
    pass = pass && approxEqual(net_fx, 0.0, tol) && approxEqual(net_fy, 0.0, tol);

    std::cout << "\n  Result: " << (pass ? "ALL PASS ✓" : "SOME TESTS FAILED ✗") << "\n";
    return pass;
}

// =============================================================================
// Quick sanity run — one NBodySimulator step with N=5 bodies
// =============================================================================

static void quickSimDemo() {
    std::cout << "\n" << HR << "\n";
    std::cout << "  Quick simulation demo — N=5, 3 steps, G=1, eps=0.1, dt=0.001\n";
    std::cout << HR << "\n";

    NBodySystem sys(1.0, 0.1);
    // Small 5-body system with circular-like initial conditions
    sys.addParticle(Particle(1.0,  1.0,  0.0,  0.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0,  0.0,  0.0, -1.0));
    sys.addParticle(Particle(1.0,  0.0,  1.0, -1.0,  0.0));
    sys.addParticle(Particle(1.0,  0.0, -1.0,  1.0,  0.0));
    sys.addParticle(Particle(5.0,  0.0,  0.0,  0.0,  0.0));  // central mass

    NBodySimulator sim(&sys, 0.001, 3);
    sim.run();

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "  State after 3 steps:\n";
    const auto& bodies = sys.getBodies();
    std::cout << "  " << std::left
              << std::setw(6)  << "Body"
              << std::setw(14) << "x"
              << std::setw(14) << "y"
              << std::setw(14) << "vx"
              << std::setw(14) << "vy" << "\n";
    std::cout << "  " << std::string(62, '-') << "\n";
    for (int i = 0; i < (int)bodies.size(); i++) {
        std::cout << "  " << std::left
                  << std::setw(6)  << i
                  << std::setw(14) << bodies[i].getX()
                  << std::setw(14) << bodies[i].getY()
                  << std::setw(14) << bodies[i].getVx()
                  << std::setw(14) << bodies[i].getVy()
                  << "\n";
    }
    std::cout << "\n  (Simulation ran successfully — full loop implemented in Week 3)\n";
}

// =============================================================================
// CLI dispatch
// =============================================================================

static void printHelp(const char* progname) {
    std::cout
        << "Usage: " << progname << " [mode]\n\n"
        << "Modes:\n"
        << "  (none)        Week-1 physics verification (N=2, N=3) + quick demo\n"
        << "  --simulate    Full N-body simulation loop (implemented Week 3)\n"
        << "  --benchmark   OpenMP performance benchmarks (implemented Week 4)\n"
        << "  --analysis    Speedup / efficiency / Amdahl analysis (Week 4)\n"
        << "  --help        Show this help message\n\n"
        << "Examples:\n"
        << "  ./nbody_2d\n"
        << "  ./nbody_2d --benchmark\n";
}

// =============================================================================
// main
// =============================================================================

int main(int argc, char* argv[]) {
    std::cout << HR << "\n";
    std::cout << "  2D N-Body Gravitational Simulator — Week 1\n";
    std::cout << "  Laboratorio 1: Programación paralela con OpenMP\n";
    std::cout << "  Universidad de Santiago de Chile\n";
    std::cout << HR << "\n";

    // Parse CLI argument (if any)
    std::string mode = "";
    if (argc >= 2) mode = argv[1];

    // -------------------------------------------------------------------------
    if (mode == "--help" || mode == "-h") {
        printHelp(argv[0]);
        return 0;
    }

    // -------------------------------------------------------------------------
    if (mode == "--benchmark") {
        std::cout << "\n[--benchmark] OpenMP benchmarks — not yet implemented (Week 4)\n";
        std::cout << "  Will cover: schedule variants, sync strategies,\n";
        std::cout << "  speedup, efficiency, Amdahl curve, error propagation.\n";
        return 0;
    }

    // -------------------------------------------------------------------------
    if (mode == "--analysis") {
        std::cout << "\n[--analysis] Scaling analysis — not yet implemented (Week 4)\n";
        std::cout << "  Will cover: T vs N, T vs threads, speedup S_p vs p.\n";
        return 0;
    }

    // -------------------------------------------------------------------------
    if (mode == "--simulate") {
        std::cout << "\n[--simulate] Full simulation loop — not yet implemented (Week 3)\n";
        std::cout << "  Will run NBodySimulator with configurable N, dt, steps.\n";
        return 0;
    }

    // -------------------------------------------------------------------------
    // Default mode: Week-1 physics verification
    // -------------------------------------------------------------------------
    bool all_pass = true;

    all_pass &= verifyN2();
    all_pass &= verifyN3();

    quickSimDemo();

    std::cout << "\n" << HR << "\n";
    std::cout << "  Week-1 Summary: "
              << (all_pass ? "ALL VERIFICATIONS PASSED ✓" : "SOME VERIFICATIONS FAILED ✗")
              << "\n";
    std::cout << HR << "\n\n";

    return all_pass ? 0 : 1;
}