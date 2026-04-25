// test_NBodySystem.cpp
// Pruebas de INTEGRACIÓN para NBodySystem y NBodySimulator.
// Usa Catch2 v3 (catch_amalgamated.hpp).
// Verifica que las clases colaboran correctamente y que la física es coherente.

#include "catch_amalgamated.hpp"
#include "../NBodySystem.h"
#include "../NBodySimulator.h"

#include <cmath>

using Catch::Approx;

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Inicialización
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: el constructor almacena G y epsilon correctamente", "[NBodySystem][init]") {
    NBodySystem sys(6.674e-11, 0.05);

    REQUIRE(sys.getG_const() == Approx(6.674e-11));
    REQUIRE(sys.getEps()     == Approx(0.05));
    REQUIRE(sys.getCount()   == 0);
}

TEST_CASE("NBodySystem: sistema vacío tiene cero partículas", "[NBodySystem][init]") {
    NBodySystem sys(1.0, 0.05);
    REQUIRE(sys.getBodies().empty());
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — addParticle
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: addParticle incrementa el contador", "[NBodySystem][addParticle]") {
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 0.0, 0.0);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 1);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 2);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 3);
}

TEST_CASE("NBodySystem: la partícula añadida conserva sus propiedades", "[NBodySystem][addParticle]") {
    NBodySystem sys(1.0, 0.05);
    Particle p(7.5, 3.0, -4.0);
    sys.addParticle(p);

    const auto& bodies = sys.getBodies();
    REQUIRE(bodies[0].getMass() == Approx(7.5));
    REQUIRE(bodies[0].getX()    == Approx(3.0));
    REQUIRE(bodies[0].getY()    == Approx(-4.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Generadores de escenarios
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: randomSystem genera exactamente N partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.randomSystem(50, 42);
    REQUIRE(sys.getCount() == 50);
}

TEST_CASE("NBodySystem: randomSystem con distinta semilla produce posiciones distintas", "[NBodySystem][generators]") {
    NBodySystem sys1(1.0, 0.05);
    NBodySystem sys2(1.0, 0.05);

    sys1.randomSystem(10, 1);
    sys2.randomSystem(10, 2);

    bool diferente = (sys1.getBodies()[0].getX() != sys2.getBodies()[0].getX()) ||
                     (sys1.getBodies()[0].getY() != sys2.getBodies()[0].getY());
    REQUIRE(diferente);
}

TEST_CASE("NBodySystem: bynarySystem genera exactamente 3 partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.bynarySystem(99);
    REQUIRE(sys.getCount() == 3);
}

TEST_CASE("NBodySystem: diskSystem genera exactamente N partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.diskSystem(30, 7);
    REQUIRE(sys.getCount() == 30);
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Física gravitacional
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: computeAccelerations produce atracción gravitacional correcta (2 cuerpos)", "[NBodySystem][physics]") {
    // p0 en (0,0), p1 en (10,0)
    // → p0 se acelera en +X (hacia p1)
    // → p1 se acelera en -X (hacia p0)
    NBodySystem sys(1.0, 0.0);

    Particle p0(1.0,  0.0, 0.0);
    Particle p1(1.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    sys.computeAccelerations();

    const auto& bodies = sys.getBodies();

    REQUIRE(bodies[0].getAX() > 0.0);
    REQUIRE(bodies[1].getAX() < 0.0);
    REQUIRE(bodies[0].getAY() == Approx(0.0));
    REQUIRE(bodies[1].getAY() == Approx(0.0));

    // 3ª Ley de Newton: masas iguales → magnitudes de aceleración iguales
    REQUIRE(std::abs(bodies[0].getAX()) == Approx(std::abs(bodies[1].getAX())));
}

TEST_CASE("NBodySystem: cuerpo masivo produce mayor aceleración en cuerpo ligero", "[NBodySystem][physics]") {
    // a = G*M/r² → p0 (masa=1) experimenta G*100/100=1.0, p1 (masa=100) experimenta G*1/100=0.01
    NBodySystem sys(1.0, 0.0);

    Particle p0(  1.0,  0.0, 0.0);
    Particle p1(100.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    sys.computeAccelerations();

    const auto& bodies = sys.getBodies();
    REQUIRE(std::abs(bodies[0].getAX()) > std::abs(bodies[1].getAX()));
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySimulator — Integración Euler
// ─────────────────────────────────────────────

TEST_CASE("NBodySimulator: partícula aislada sin fuerzas no se mueve", "[NBodySimulator][euler]") {
    // Un solo cuerpo → sin interacciones → aceleración = 0 → no se mueve
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 5.0, 7.0);
    sys.addParticle(p);

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() == Approx(5.0));
    REQUIRE(sys.getParticles()[0].getY() == Approx(7.0));
}

TEST_CASE("NBodySimulator: partícula con velocidad inicial se desplaza correctamente", "[NBodySimulator][euler]") {
    // p con vx=1.0, ay=0 → tras dt=0.1: x = 0 + 1.0*0.1 = 0.1
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 0.0, 0.0);
    sys.addParticle(p);
    sys.getParticles()[0].setVelocity(1.0, 0.0);

    NBodySimulator sim(&sys, 0.1);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() == Approx(0.1));
    REQUIRE(sys.getParticles()[0].getY() == Approx(0.0));
}

TEST_CASE("NBodySimulator: dos partículas se acercan tras un paso de integración", "[NBodySimulator][euler]") {
    // p0 en x=0, p1 en x=10 → se atraen → después de un paso: x0>0, x1<10
    NBodySystem sys(1.0, 0.0);
    Particle p0(1.0,  0.0, 0.0);
    Particle p1(1.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() > 0.0);
    REQUIRE(sys.getParticles()[1].getX() < 10.0);
}
