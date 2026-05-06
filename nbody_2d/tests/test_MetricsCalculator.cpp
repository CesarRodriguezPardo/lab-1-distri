// test_MetricsCalculator.cpp
// ─────────────────────────────────────────────────────────────────────────────
// Tests unitarios para MetricsCalculator.
// Cubre: calculateEnergy (method 0 y 1), calculateMetricsFirstprivate(),
//        calculateFinalStateLastprivate() — center-of-mass, momentum, RMS radius
//        y distancia mínima.
//
// Usa Catch2 v3 (catch_amalgamated.hpp).
// ─────────────────────────────────────────────────────────────────────────────

#include "catch_amalgamated.hpp"
#include "../MetricsCalculator.h"
#include "../NBodySystem.h"
#include "../Particle.h"

#include <cmath>

using Catch::Approx;

// ─────────────────────────────────────────────
//  MC-1 · calculateEnergy(method=0)
//         energía potencial y cinética K=0
// ─────────────────────────────────────────────
// Sistema: 2 cuerpos en reposo
//   p0: masa=1, pos=(0,0), vel=(0,0)
//   p1: masa=1, pos=(3,4), vel=(0,0)
//   G=1.0, eps=0.0
//
// Analítico:
//   r  = sqrt(3² + 4²) = 5.0
//   K  = 0.0   (ambos en reposo)
//   U  = -G·m0·m1 / r = -1·1·1 / 5 = -0.2
//   E  = K + U = -0.2
TEST_CASE("MetricsCalculator MC-1: energia potencial con 2 cuerpos en reposo (method=0)",
          "[MetricsCalculator][energy][MC-1]") {
    NBodySystem sys(1.0, 0.0);   // G=1, eps=0
    Particle p0(1.0, 0.0, 0.0);
    Particle p1(1.0, 3.0, 4.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    MetricsCalculator mc(&sys);
    mc.calculateEnergy(0);

    REQUIRE(mc.getKineticEnergy()   == Approx(0.0).margin(1e-10));
    REQUIRE(mc.getPotentialEnergy() == Approx(-0.2).margin(1e-10));
    REQUIRE(mc.getTotalEnergy()     == Approx(-0.2).margin(1e-10));
}

// ─────────────────────────────────────────────
//  MC-2 · calculateEnergy(method=0)
//         energía cinética pura (sin pares → U=0)
// ─────────────────────────────────────────────
// Sistema: 1 cuerpo aislado
//   p0: masa=2.0, pos=(0,0), vel=(3,4)
//   G=1.0, eps=0.0
//
// Analítico:
//   K = 0.5 · 2 · (3² + 4²) = 0.5 · 2 · 25 = 25.0
//   U = 0.0  (no hay pares i<j)
//   E = 25.0
TEST_CASE("MetricsCalculator MC-2: energia cinetica con 1 cuerpo aislado (method=0)",
          "[MetricsCalculator][energy][MC-2]") {
    NBodySystem sys(1.0, 0.0);
    Particle p0(2.0, 0.0, 0.0);
    sys.addParticle(p0);
    sys.getParticles()[0].setVelocity(3.0, 4.0);

    MetricsCalculator mc(&sys);
    mc.calculateEnergy(0);

    REQUIRE(mc.getKineticEnergy()   == Approx(25.0).margin(1e-10));
    REQUIRE(mc.getPotentialEnergy() == Approx(0.0).margin(1e-10));
    REQUIRE(mc.getTotalEnergy()     == Approx(25.0).margin(1e-10));
}

// ─────────────────────────────────────────────
//  MC-3 · calculateEnergy(method=1)
//         consistencia con method=0
// ─────────────────────────────────────────────
// Mismo sistema que MC-1. Verifica que el método basado en padding
// (anti-false-sharing) produce resultados idénticos al de reducción estándar.
TEST_CASE("MetricsCalculator MC-3: method=1 coincide con method=0 (padding anti-false-sharing)",
          "[MetricsCalculator][energy][MC-3]") {
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
//  MC-4 · calculateMetricsFirstprivate()
//         centro de masa de 2 cuerpos iguales
// ─────────────────────────────────────────────
// Sistema: 2 cuerpos de masa igual en reposo
//   p0: masa=1, pos=(0,0)
//   p1: masa=1, pos=(10,0)
//
// Analítico:
//   CM_x = (1·0 + 1·10) / (1+1) = 5.0
//   CM_y = 0.0
TEST_CASE("MetricsCalculator MC-4: centro de masa de 2 cuerpos de masa igual",
          "[MetricsCalculator][metrics][MC-4]") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();

    REQUIRE(mc.getCmX() == Approx(5.0).margin(1e-10));
    REQUIRE(mc.getCmY() == Approx(0.0).margin(1e-10));
}

// ─────────────────────────────────────────────
//  MC-5 · calculateMetricsFirstprivate()
//         momentum total nulo (momenta opuestos)
// ─────────────────────────────────────────────
// Sistema: 2 cuerpos con momentum opuesto
//   p0: masa=2, vel=(3,0)  → px = 6
//   p1: masa=1, vel=(-6,0) → px = -6
//
// Analítico:
//   px = 2·3 + 1·(-6) = 0
//   py = 0
//   momentumMagnitude = 0.0
TEST_CASE("MetricsCalculator MC-5: momentum total nulo con momentos opuestos",
          "[MetricsCalculator][metrics][MC-5]") {
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
//  MC-6 · calculateFinalStateLastprivate()
//         RMS radius de 4 cuerpos en cuadrado simétrico
// ─────────────────────────────────────────────
// Requiere calcular CM primero.
// Sistema: 4 cuerpos de masa=1 en cuadrado centrado en origen
//   p0=(1,1), p1=(-1,1), p2=(-1,-1), p3=(1,-1)
//
// Analítico:
//   CM = (0, 0)  (por simetría)
//   sumSqDist = 4 · (1² + 1²) = 8.0
//   totalMass = 4.0
//   rmsRadius = sqrt(8/4) = sqrt(2) ≈ 1.41421356...
TEST_CASE("MetricsCalculator MC-6: RMS radius de 4 cuerpos en cuadrado simetrico",
          "[MetricsCalculator][finalstate][MC-6]") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  1.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0,  1.0));
    sys.addParticle(Particle(1.0, -1.0, -1.0));
    sys.addParticle(Particle(1.0,  1.0, -1.0));

    MetricsCalculator mc(&sys);
    // calculateFinalStateLastprivate() usa cmX/cmY calculados previamente
    mc.calculateMetricsFirstprivate();
    mc.calculateFinalStateLastprivate();

    REQUIRE(mc.getRmsRadius() == Approx(std::sqrt(2.0)).margin(1e-10));
}

// ─────────────────────────────────────────────
//  MC-7 · calculateFinalStateLastprivate()
//         distancia mínima — 3 cuerpos colineales
// ─────────────────────────────────────────────
// Sistema: 3 cuerpos colineales
//   p0=(0,0), p1=(3,0), p2=(10,0)
//
// Distancias inter-cuerpo:
//   d(0,1) = 3.0
//   d(0,2) = 10.0
//   d(1,2) = 7.0
//   minDistance = 3.0
TEST_CASE("MetricsCalculator MC-7: distancia minima de 3 cuerpos colineales",
          "[MetricsCalculator][finalstate][MC-7]") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0,  3.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    MetricsCalculator mc(&sys);
    mc.calculateMetricsFirstprivate();   // necesario para cmX/cmY antes de llamar a lastprivate
    mc.calculateFinalStateLastprivate();

    REQUIRE(mc.getMinDistance() == Approx(3.0).margin(1e-10));
}
