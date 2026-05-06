// test_MetricsCalculator.cpp
// ─────────────────────────────────────────────────────────────────────────────
// GUÍA GENERAL DE TOLERANCIAS
// ─────────────────────────────────────────────────────────────────────────────
// Los valores analíticos se calculan con aritmética exacta de racionales
// simples. El error de representación IEEE-754 en doble precisión es ~1e-15.
// Usamos margin(1e-10) para acomodar errores de redondeo acumulados en las
// sumas paralelas (reduction), que pueden diferir levemente del orden serial.
// Pruebas UNITARIAS para MetricsCalculator.
// Usa Catch2 v3 (catch_amalgamated.hpp).
// ─────────────────────────────────────────────────────────────────────────────

#include "catch_amalgamated.hpp"
#include "../NBodySystem.h"
#include "../MetricsCalculator.h"

#include <cmath>

using Catch::Approx;

// ─────────────────────────────────────────────
//  SECCIÓN: calculateEnergy — método reduction (method=0)
// ─────────────────────────────────────────────

TEST_CASE("MetricsCalculator: calculateEnergy(0) — K=0 con cuerpos en reposo",
          "[MetricsCalculator][energy][method0]") {
    // Geometría fija:
    //   p0 en (0,0)  masa=1, vel=(0,0)
    //   p1 en (3,4)  masa=1, vel=(0,0)
    //   G=1.0, eps=0.0
    //
    // Analítico:
    //   r = sqrt(3²+4²) = 5.0
    //   K = 0.0  (velocidades nulas)
    //   U = -G·m0·m1 / r = -1·1·1 / 5 = -0.2
    //   E = -0.2

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 3.0, 4.0));

    MetricsCalculator mc(&sys);
    mc.calculateEnergy(0);

    REQUIRE(mc.getKineticEnergy()   == Approx( 0.0).margin(1e-10));
    REQUIRE(mc.getPotentialEnergy() == Approx(-0.2).margin(1e-10));
    REQUIRE(mc.getTotalEnergy()     == Approx(-0.2).margin(1e-10));
}

TEST_CASE("MetricsCalculator: calculateEnergy(0) — K pura con un cuerpo aislado",
          "[MetricsCalculator][energy][method0]") {
    // Un solo cuerpo → no hay pares i<j → U = 0
    //   p0: masa=2.0, vel=(3,4)
    //
    // Analítico:
    //   |v|² = 3²+4² = 25
    //   K = 0.5 · 2.0 · 25 = 25.0
    //   U = 0.0
    //   E = 25.0

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(2.0, 0.0, 0.0));
    sys.getParticles()[0].setVelocity(3.0, 4.0);

    MetricsCalculator mc(&sys);
    mc.calculateEnergy(0);

    REQUIRE(mc.getKineticEnergy()   == Approx(25.0).margin(1e-10));
    REQUIRE(mc.getPotentialEnergy() == Approx( 0.0).margin(1e-10));
    REQUIRE(mc.getTotalEnergy()     == Approx(25.0).margin(1e-10));
}

// ─────────────────────────────────────────────
//  SECCIÓN: calculateEnergy — consistencia method=0 vs method=1
// ─────────────────────────────────────────────
// method=1 usa padding para evitar false sharing.
// Debe producir exactamente los mismos resultados que method=0.
// Si el padding introduce un bug (e.g., off-by-one en el vector de
// acumuladores), los resultados diferirán.

TEST_CASE("MetricsCalculator: calculateEnergy — method=1 coincide con method=0",
          "[MetricsCalculator][energy][method1]") {
    // Mismo sistema que MC-1: 2 cuerpos en reposo
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 3.0, 4.0));

    MetricsCalculator mc(&sys);

    mc.calculateEnergy(0);
    double K0 = mc.getKineticEnergy();
    double U0 = mc.getPotentialEnergy();

    mc.calculateEnergy(1);
    double K1 = mc.getKineticEnergy();
    double U1 = mc.getPotentialEnergy();

    REQUIRE(K1 == Approx(K0).margin(1e-10));
    REQUIRE(U1 == Approx(U0).margin(1e-10));
}

// ─────────────────────────────────────────────
//  SECCIÓN: calculateMetricsFirstprivate — centro de masa
// ─────────────────────────────────────────────

TEST_CASE("MetricsCalculator: calculateMetricsFirstprivate — centro de masa de 2 cuerpos iguales",
          "[MetricsCalculator][firstprivate][CM]") {
    // p0 en (0,0), p1 en (10,0), masas iguales
    // CM_x = (1·0 + 1·10) / 2 = 5.0
    // CM_y = 0.0

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();

    REQUIRE(mc.getCmX() == Approx(5.0).margin(1e-10));
    REQUIRE(mc.getCmY() == Approx(0.0).margin(1e-10));
}

TEST_CASE("MetricsCalculator: calculateMetricsFirstprivate — momentum total nulo",
          "[MetricsCalculator][firstprivate][momentum]") {
    // p0: masa=2, vel=(3,0)  → px=6
    // p1: masa=1, vel=(-6,0) → px=-6
    // Suma: px=0, py=0 → |p|=0
    //
    // Detecta: si firstprivate inicializa mal las variables de momentum.

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(2.0, 0.0, 0.0));
    sys.addParticle(Particle(1.0, 5.0, 0.0));
    sys.getParticles()[0].setVelocity( 3.0, 0.0);
    sys.getParticles()[1].setVelocity(-6.0, 0.0);

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();

    REQUIRE(mc.getMomentumMagnitude() == Approx(0.0).margin(1e-10));
}

// ─────────────────────────────────────────────
//  SECCIÓN: calculateFinalStateLastprivate — RMS radius y distancia mínima
// ─────────────────────────────────────────────
// NOTA: calculateFinalStateLastprivate() usa cmX/cmY (miembros) para
// calcular el radio RMS. Por eso se llama a calculateMetricsFirstprivate()
// primero, igual que lo hace calculateAllMetrics() en producción.

TEST_CASE("MetricsCalculator: calculateFinalStateLastprivate — RMS radius en cuadrado simétrico",
          "[MetricsCalculator][lastprivate][rms]") {
    // 4 cuerpos de masa=1 en cuadrado simétrico centrado en el origen.
    //   p0=(1,1), p1=(-1,1), p2=(-1,-1), p3=(1,-1)
    //
    // Por simetría: CM = (0,0)
    //
    // sumSqDist = Σ m_i·(dx²+dy²) = 4·(1²+1²) = 8.0
    // totalMass = 4.0
    // rmsRadius = sqrt(8/4) = sqrt(2) ≈ 1.41421356...

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  1.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0, -1.0));
    sys.addParticle(Particle(1.0,  1.0, -1.0));

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();        // fija cmX=0, cmY=0
    mc.calculateFinalStateLastprivate();

    REQUIRE(mc.getRmsRadius() == Approx(std::sqrt(2.0)).margin(1e-10));
}

TEST_CASE("MetricsCalculator: calculateFinalStateLastprivate — distancia mínima entre cuerpos",
          "[MetricsCalculator][lastprivate][minDist]") {
    // 3 cuerpos colineales: p0=(0,0), p1=(3,0), p2=(10,0)
    //
    // Distancias:
    //   d(0,1) = 3.0
    //   d(0,2) = 10.0
    //   d(1,2) = 7.0
    // minDistance = 3.0

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0,  3.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();       // necesario para inicializar cmX/cmY
    mc.calculateFinalStateLastprivate();

    REQUIRE(mc.getMinDistance() == Approx(3.0).margin(1e-10));
}
